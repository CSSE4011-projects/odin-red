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

/* Initialising message queue for control data */
K_MSGQ_DEFINE(
    control_msgq,
    sizeof(struct control_data),
    CONTROL_MSGQ_MAX_MSG,
    CONTROL_MSGQ_ALIGN
);

/* Initialising message queue for position data */
K_MSGQ_DEFINE(
    pos_msgq,
    sizeof(struct pos_data),
    POS_MSGQ_MAX_MSG,
    POS_MSGQ_ALIGN
);

static struct bt_conn *default_conn;
bool bt_connected;

LOG_MODULE_REGISTER(app);

uint8_t current_cont[] = {0, 0, 0};
uint8_t pos_data[] = {0, 0};

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
    if (err) {
		LOG_ERR("Connection failed (err 0x%02x)\n", err);
        bt_connected = false;
	} else {
		LOG_INF("Connected");
        bt_connected = true;
	}
    default_conn = conn;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected");
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
    if (length == 3 && bt_connected) {
        memcpy(&current_cont, data, 3);
        printk("%d %d %d\n", current_cont[0], current_cont[1], current_cont[2]);
    }
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
    err = bt_enable(NULL);
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

    /* Struct to store all control values */
    struct control_data control;

    /* Struct to store position values */
    struct pos_data position;

    while (1)
    {
        if (bt_connected) {
            bt_gatt_read(default_conn, &read_cont_params);

            control.pedal_left = current_cont[0];
            control.pedal_right = current_cont[1];
            control.rudder_angle = current_cont[2];

            /* Send control data to main */
            if (k_msgq_put(&control_msgq, &control, K_NO_WAIT) != 0) {
                /* Queue is full, purge it */
                k_msgq_purge(&control_msgq);
            }

            // Check for updated pos from main
            if (!k_msgq_get(&pos_msgq, &position, K_NO_WAIT)) {
                pos_data[0] = position.x_pos;
                pos_data[1] = position.y_pos;
            }

        }

        k_msleep(500);
    }
}