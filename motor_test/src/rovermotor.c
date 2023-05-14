#include "rovermotor.h"


LOG_MODULE_REGISTER(rovermotor);

K_THREAD_STACK_DEFINE(rovermotor_handler_stack, ROVERMOTOR_STACK_SIZE);

/**
 * Initializes parameters stored in rovermotor handle motor_info. 
 * Creates handler thread and returns its thread ID. 
 */
k_tid_t rovermotor_init(
            uint8_t left_addr, 
            uint8_t right_addr, 
            const struct device* dev, 
            struct rovermotor_info* motor_info
            ) 
{
    motor_info->left_addr = left_addr;
    motor_info->right_addr = right_addr;
    motor_info->dev = dev; 
    LOG_INF("Creating queue");
    k_sleep(K_MSEC(1000));
    
    k_msgq_init(
        &(motor_info->handler_queue), 
        motor_info->cmd_msg_buf, 
        sizeof(struct rover_queue_msg), 
        16
        );

    k_sleep(K_MSEC(1000));

    // Create thread and pass in handler struct to avoid global. 

    LOG_INF("Creating thread to handle rover motor control");
    return k_thread_create(&(motor_info->motor_thread_data), rovermotor_handler_stack,
		K_THREAD_STACK_SIZEOF(rovermotor_handler_stack),
		rovermotor_handler,
		(void*) (motor_info), NULL, NULL,
		ROVERMOTOR_PRIORITY, 0, K_NO_WAIT);
}


void rovermotor_handler(void* handler, void*, void* )
{
    struct rovermotor_info info = *((struct rovermotor_info*) handler); 
    struct rover_queue_msg instruction; 
    int msg_rx_status; 
    // Initialize left and right motors. 
    LOG_INF("Initializing left motor");
    motordriver_init(info.dev, info.left_addr);
    LOG_INF("Initializing right motor");
    motordriver_init(info.dev, info.right_addr); 

    // Block until a message comes in, process, update motor control. 
    while (1) 
    {
        msg_rx_status = k_msgq_get(&(info.handler_queue), (void*) (&instruction), K_FOREVER);
        if (msg_rx_status) {
            LOG_INF("Couldn't get command from queue, %i", msg_rx_status); 
        } else {
            // We have a command to instruct the motor driver. 
            // Set angular velocity: 
            
            int left = instruction.direction * ANGLE_CHANGE_RATE_CONST; 
            int right = left * -1; 

            // Adjust for forward velocity: 
            left += ((int) (instruction.velocity)) * FWD_VELOCITY_RATE_CONST;
            right += ((int) (instruction.velocity)) * FWD_VELOCITY_RATE_CONST; 
            LOG_INF("Left: %i, right: %i", left, right);
            uint8_t left_motor_cmd = left + 127; 
            uint8_t right_motor_cmd = right + 127; 
            LOG_INF("Motor commands: %hhu, %hhu", left_motor_cmd, right_motor_cmd);
            motordriver_send_pwm(info.dev, info.left_addr, left_motor_cmd);
            motordriver_send_pwm(info.dev, info.right_addr, right_motor_cmd); 
            

           

        }
    }
    
}

/**
 * Sends instruction to rover motor control handler thread, to move rover 
 * forward at given speed, and to turn at given direction speed
 * @require direction if from -1 to 1, mapping angular velocity (like a percentage). 
 */
int rovermotor_send_instruction(
        struct rovermotor_info* motor_info, 
        float direction, 
        int8_t speed) 
{
    struct rover_queue_msg command = {
        .direction = direction,
        .velocity = speed,
    };

    int status = k_msgq_put(&(motor_info->handler_queue), (void*) (&command), K_NO_WAIT);
    if (status == -ENOMSG) {
        LOG_ERR("Couldn't send to handler: ENOMSG: Returned without waiting or queue purged.");
    } else if (status == -EAGAIN) {
        LOG_ERR("Couldn't send to handler: EAGAIN: Waiting period timed out.");
    } else if (status) {
        LOG_ERR("Couldn't send to handler: unknown error");
    }
    return status; 
}