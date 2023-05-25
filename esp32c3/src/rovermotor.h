#ifndef _ROVERMOTOR_H_
#define _ROVERMOTOR_H_
#include <stdlib.h>
#include <zephyr/kernel.h>
#define ROVERMOTOR_STACK_SIZE 4096
#define ROVERMOTOR_PRIORITY 14
#define FWD_VELOCITY_RATE_CONST 1
#define ANGLE_CHANGE_RATE_CONST 80
#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include "motordriver.h"


/** 
 * Handler to store control information on rover
 * motor control. 
 */
struct rovermotor_info 
{
    /* i2c addr of left motor controller */
    uint8_t left_addr;
    /* i2c addr of right motor controller */
    uint8_t right_addr;
    /* queue for instructions to drive motors*/
    struct k_msgq handler_queue;
    /* internal */
    char __aligned(4) cmd_msg_buf[16 * sizeof(float)];
    /* i2c periperhal device */
    const struct device* dev; 
    /* thread performing motor control */
    struct k_thread motor_thread_data; 
};


/* Single message to send to motors. */
struct rover_queue_msg 
{
    /* Mapping -1 to +1 for maximum left turn rate 
    * to maximum right turn rate. */
    float direction;
    /* Velocity in forward direction */
    int8_t velocity; 
};


/**
 * Initializes parameters stored in rovermotor handle motor_info. 
 * Creates handler thread and returns its thread ID. 
 * @param left_addr 7-bit i2c address of left-side motor controller
 * @param right_addr 7 bit i2c address of right side motor controller
 * @param dev zephyr i2c device that each motor driver is run on. 
 * @param motor_info uninitialized struct (it is initialized in this function). 
 */
extern k_tid_t rovermotor_init (
            uint8_t left_addr, 
            uint8_t right_addr, 
            const struct device* dev, 
            struct rovermotor_info* motor_info
            );


/**
 * Sends instruction to rover motor control handler thread, to move rover 
 * forward at given speed, and to turn at given direction speed
 * @require direction if from -1 to 1, mapping angular velocity (like a percentage). 
 */
extern int rovermotor_send_instruction(
        struct rovermotor_info* motor_info, 
        float direction, 
        int8_t speed);
        

/**
 * Hanlder thread for rover motor control. Blocks until a message is received from 
 * another task (from call to rovermotor_send_instruction). 
 * First parameter is pointer to handler struct rovermotor_info
 */
void rovermotor_handler(void* handler, void*, void* );

#endif 