#ifndef BSU_BT_H
#define BSU_BT_H

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <version.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include "pos_base_hci.h"

/* Macro definitions for control message queue */
#define CONTROL_MSGQ_MAX_MSG 10
#define CONTROL_MSGQ_ALIGN 4

void bt_th(struct Data*);
void bt_read(void);

/* Message queue for control data */
extern struct k_msgq control_msgq;

/* Struct for control data */
struct control_data {
    uint8_t pedal_left;
    uint8_t pedal_right;
    uint8_t rudder_angle;
};

#endif