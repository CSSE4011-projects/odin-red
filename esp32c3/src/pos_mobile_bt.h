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

/* Macro definitions for control message queue */
#define CONTROL_MSGQ_MAX_MSG 10
#define CONTROL_MSGQ_ALIGN 4

/* Macro definitions for position message queue */
#define POS_MSGQ_MAX_MSG 10
#define POS_MSGQ_ALIGN 4

void ble_connect_main(void);

/* Message queue for control data */
extern struct k_msgq control_msgq;

/* Message queue for position data */
extern struct k_msgq pos_msgq;

/* Struct for control data */
struct control_data {
    uint8_t pedal_left;
    uint8_t pedal_right;
    uint8_t rudder_angle;
};

/* Struct for position data */
struct pos_data {
    uint8_t x_pos;
    uint8_t y_pos;
};

#endif