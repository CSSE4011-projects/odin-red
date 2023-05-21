/*
* Driver for Base node bluetooth
*/

#include "pos_base_bt.h"

LOG_MODULE_REGISTER(bt);

/* Initialising message queue for control data */
K_MSGQ_DEFINE(
    control_msgq,
    sizeof(struct control_data),
    CONTROL_MSGQ_MAX_MSG,
    CONTROL_MSGQ_ALIGN
);

struct k_msgq *bt_queue;
struct k_event *bt_ev;
struct k_event data_ev;
bool get_node_params;
sys_slist_t* bt_list;
bool ble_connected;

#define UUID_BUFFER_SIZE 16

static void start_scan(void);

uint8_t current_pos[] = {0, 0};
uint8_t cont_data[] = {0, 0, 0};

static struct bt_conn *default_conn;

uint16_t mobile_uuid[] = {0xd0, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
                          0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcb};

uint16_t base_uuid[] = {0xd0, 0x92, 0x27, 0x35, 0x78, 0x16, 0x21, 0x91,
                          0x26, 0x49, 0x60, 0xeb, 0x06, 0xa3, 0xca, 0x2b};

static struct bt_uuid_128 cont_uuid = BT_UUID_INIT_128(
    0xd8, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcb);

static struct bt_uuid_128 pos_uuid = BT_UUID_INIT_128(
    0xd8, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcb);


static bool check_dev(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
    int i;
    int matchedCount = 0;

    if (data->type == BT_DATA_UUID128_ALL)
    {

        uint16_t temp = 0;
        for (i = 0; i < data->data_len; i++)
        {
            temp = data->data[i];
            if (temp == mobile_uuid[i])
                matchedCount++;
        }

        if (matchedCount == UUID_BUFFER_SIZE)
        {
            //MOBILE UUID MATCHED
            LOG_INF("Mobile UUID Found, attempting to connect\n");

            int err = bt_le_scan_stop();
            k_msleep(10);

            if (err)
            {
                LOG_INF("Stop LE scan failed (err %d)\n", err);
                return true;
            }

            struct bt_le_conn_param *param = BT_LE_CONN_PARAM_DEFAULT;

            err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
                                    param, &default_conn);
            if (err)
            {
                LOG_INF("Create conn failed (err %d)\n", err);
                start_scan();
            }

            return false;
        }
    }
    return true;
}

static ssize_t give_cont(struct bt_conn *conn,
                          const struct bt_gatt_attr *attr, void *buf,
                          uint16_t len, uint16_t offset)
{
    const int16_t *value = attr->user_data;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(cont_data));
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad)
{
	if (type == BT_GAP_ADV_TYPE_ADV_IND ||
        type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND)
    {
        bt_data_parse(ad, check_dev, (void *)addr);
    }
}

uint8_t read_pos(struct bt_conn *conn, uint8_t err,
                               struct bt_gatt_read_params *params,
                               const void *data, uint16_t length)
{
    memcpy(current_pos, data, 2);
    return 0;
}


BT_GATT_SERVICE_DEFINE(base_svc,
                       BT_GATT_PRIMARY_SERVICE(&base_uuid),
                       
                       BT_GATT_CHARACTERISTIC(&cont_uuid.uuid,
                                              BT_GATT_CHRC_READ,
                                              BT_GATT_PERM_READ,
                                              give_cont, NULL, &cont_data),  );

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		LOG_INF("Scanning failed to start (err %d)\n", err);
		return;
	}

	LOG_INF("Scanning successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err)
    {
        LOG_INF("Failed to connect to %s (%u)\n", addr, err);

        bt_conn_unref(default_conn);
        default_conn = NULL;

        start_scan();
        return;
    }

    if (conn != default_conn)
    {
        return;
    }
    ble_connected = true;
    LOG_INF("Connected: %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

    if (conn != default_conn)
        return;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected: %s (reason 0x%02x)\n", addr, reason);

    bt_conn_unref(default_conn);
    default_conn = NULL;
    ble_connected = false;
    start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

void bt_read(void)
{
    static struct bt_gatt_read_params read_pos_params = {
        .func = read_pos,
        .handle_count = 0,
        .by_uuid.uuid = &pos_uuid.uuid,
        .by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
        .by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
    };

    k_msleep(1000);

    struct control_data control;

    while (1)
    {
        if (ble_connected)
        {
            bt_gatt_read(default_conn, &read_pos_params);

            printk("{%d, %d}\n", current_pos[0], current_pos[1]);

            // Check for updated pos from main
            if (!k_msgq_get(&control_msgq, &control, K_NO_WAIT)) {
                cont_data[0] = control.pedal_left;
                cont_data[1] = control.pedal_right;
                cont_data[2] = control.rudder_angle;
            }
        }
        k_msleep(200);
    }

}


void bt_th(struct Data * input)
{

    sys_slist_init(input->n_list);
    bt_list = input->n_list;
	bt_queue = input->msgq;
    bt_ev = input->event;

	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_INF("Bluetooth init failed (err %d)\n", err);
		return;
	}

    start_scan();

    while (1) {
        k_sleep(K_MSEC(500));
    }
}
