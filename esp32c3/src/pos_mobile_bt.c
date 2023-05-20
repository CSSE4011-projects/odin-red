#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <version.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/zephyr.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <zephyr/types.h>

#include "pos_mobile_bt.h"

#ifdef CONFIG_ARCH_POSIX
#include <unistd.h>
#else
#include <zephyr/posix/unistd.h>
#endif


#define LED0_NODE DT_ALIAS(led0)
static struct bt_conn *default_conn;
float current_range;
bool ndata_ready;
bool data_ready;
bool esp0_conn_ack;
bool esp1_conn_ack;
bool restart_scan;

uint16_t maj_num;

uint8_t num_operational;

uint8_t ultra_val;

uint8_t current_esp;
uint8_t current_esp_setup;


bool bt_connected;

uint8_t current_rssi;

static uint8_t receive[5];

const uint16_t unset_major = 0xFFFF;


int data_type;

#define STACKSIZE 2048
#define PRIORITY 7
#define MSG_SIZE 32
#define PI 3.141592654


/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

LOG_MODULE_REGISTER(app);

struct k_event sens;

int msg_len;
int period;

static int64_t time_stamp;
uint8_t tx_data[10];

uint16_t current_cont;
int node_set;
int connect_attempt;

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

float pos_data[] = {0};

int16_t rx_remp[20];
uint8_t ultra_data[] = {0, 0, 0, 0};



static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                  0xd0, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
                  0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcb),
};

static struct bt_uuid_128 mobile_uuid = BT_UUID_INIT_128(
    0xd0, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcb);

static struct bt_uuid_128 pos_uuid = BT_UUID_INIT_128(
    0xd8, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcb);

static struct bt_uuid_128 cont_uuid = BT_UUID_INIT_128(
    0xd8, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcb);



static ssize_t give_pos(struct bt_conn *conn,
                          const struct bt_gatt_attr *attr, void *buf,
                          uint16_t len, uint16_t offset)
{
    const int16_t *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(pos_data));
}

BT_GATT_SERVICE_DEFINE(mobile_svc,
                       BT_GATT_PRIMARY_SERVICE(&mobile_uuid),
                       
                       BT_GATT_CHARACTERISTIC(&pos_uuid.uuid,
                                              BT_GATT_CHRC_READ,
                                              BT_GATT_PERM_READ,
                                              give_pos, NULL, &pos_data),  );


static void connected(struct bt_conn *conn, uint8_t err)
{
    default_conn = conn;
	bt_connected = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bt_connected = false;
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	k_msleep(50);
}

uint8_t read_cont(struct bt_conn *conn, uint8_t err,
                               struct bt_gatt_read_params *params,
                               const void *data, uint16_t length)
{
    memcpy(&current_cont, data, 2);
    printk("%d\n", current_cont);
    return 0;
}


static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

static struct bt_conn_auth_cb auth_cb_display = {
    .passkey_display = auth_passkey_display,
    .passkey_entry = NULL,
    .cancel = auth_cancel,
};

static void bt_ready(void)
{
    int err = 0;
    // bt enable
    if (err)
    {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");
    /*
    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }
    */
    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);

    if (err)
    {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }
    printk("Advertising successfully started\n");
}

void ble_connect_main(void)
{
	bt_ready();
    

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&auth_cb_display);


    static struct bt_gatt_read_params read_cont_params = {
        .func = read_cont,
        .handle_count = 0,
        .by_uuid.uuid = &cont_uuid.uuid,
        .by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
        .by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
    };

    while (1)
    {
        if (bt_connected) {
            bt_gatt_read(default_conn, &read_cont_params);
        }
        k_msleep(100);
    }
}