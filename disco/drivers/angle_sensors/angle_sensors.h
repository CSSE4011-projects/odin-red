
#ifndef ANGLE_SENSORS_H_
#define ANGLE_SENSORS_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <math.h>

/* Macro definitions for angle sensors thread */
#define ANGLE_THREAD_STACK_SIZE 1024
#define ANGLE_THREAD_PRIORITY 2

/* Macro definitions for angle sensors message queue */
#define ANGLE_MSGQ_MAX_MSG 10
#define ANGLE_MSGQ_ALIGN 4

/* Entry point to angle sensors thread */
extern void angle_sensors_thread(void *, void *, void *);

/* Struct to store calculated angle */
struct angle_data {
    double angle;
};

/* Message queue for angle sensor data */
extern struct k_msgq angle_msgq;

/* Semaphore for signalling reference angle updates */
extern struct k_sem update_ref_angle_sem;

#endif
