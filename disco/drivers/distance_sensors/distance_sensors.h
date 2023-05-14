
#ifndef DISTANCE_SENSORS_H_
#define DISTANCE_SENSORS_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/printk.h>

/* Macro definitions for distance sensors thread */
#define DISTANCE_THREAD_STACK_SIZE 1024
#define DISTANCE_THREAD_PRIORITY 2

/* Macro definitions for distance sensors message queue */
#define DISTANCE_MSGQ_MAX_MSG 10
#define DISTANCE_MSGQ_ALIGN 4

/* Entry point to distance sensors thread */
extern void distance_sensors_thread(void *, void *, void *);

/* Struct to store distances */
struct distance_data {
    double front_distance;
    double back_distance;
    double left_distance;
    double right_distance;
};

/* Message queue for distance sensor data */
extern struct k_msgq distances_msgq;

#endif