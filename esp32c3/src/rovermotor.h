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

struct rovermotor_info 
{
    uint8_t left_addr;
    uint8_t right_addr;
    struct k_msgq handler_queue;
    char __aligned(4) cmd_msg_buf[16 * sizeof(float)];
    const struct device* dev; 
    struct k_thread motor_thread_data; 
};

struct rover_queue_msg 
{
    float direction;
    int8_t velocity; 
};

extern k_tid_t rovermotor_init (
            uint8_t left_addr, 
            uint8_t right_addr, 
            const struct device* dev, 
            struct rovermotor_info* motor_info
            );

extern int rovermotor_send_instruction(
        struct rovermotor_info* motor_info, 
        float direction, 
        int8_t speed);
        
void rovermotor_handler(void* handler, void*, void* );

#endif 