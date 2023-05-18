
#ifndef SERIAL_COMMS_H_
#define SERIAL_COMMS_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

/* Macro definitions for SCU hci thread */
#define SERIAL_COMMS_STACK_SIZE 1024
#define SERIAL_COMMS_PRIORITY 3

/* Macro definitions for serial comms outgoing message queue */
#define SERIAL_COMMS_MSGQ_MAX_MSG 20
#define SERIAL_COMMS_MSGQ_ALIGN 4

/* Macro definitions for UART buffer */
#define MSG_SIZE 32
#define PREAMBLE 0xFF
#define UPDATE_ANGLE_BYTE 0x69

/* Entry point to SCU hci thread */
extern void serial_comms_thread(void *, void *, void *);

/* Struct to store hci requests (outgoing from hci thread) */
struct serial_comms_data {
    uint8_t x_position;
    uint8_t y_position;
};

/* Message queue outgoing serial comms */
extern struct k_msgq serial_comms_msgq;

/* Semaphore for signalling reference angle updates */
extern struct k_sem serial_ref_angle_sem;

#endif
