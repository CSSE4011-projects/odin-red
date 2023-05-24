#ifndef BSU_BT_H
#define BSU_BT_H

#include <zephyr/kernel.h>

#include <zephyr/types.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/shell/shell.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <stddef.h>
#include <errno.h>
#include <version.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>


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
    /* 0 <= RUDDER <= 180*/
    uint8_t rudder_angle;
};

/* Struct for position data */
struct pos_data {
    uint8_t x_pos;
    uint8_t y_pos;
};

/* Semaphore for signalling reference angle updates */
extern struct k_sem update_ref_angle_sem;

#endif