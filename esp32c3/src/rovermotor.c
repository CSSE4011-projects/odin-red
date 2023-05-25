#include "rovermotor.h"
LOG_MODULE_REGISTER(rovermotor);

K_THREAD_STACK_DEFINE(rovermotor_handler_stack, ROVERMOTOR_STACK_SIZE);

/**
 * Initializes parameters stored in rovermotor handle motor_info. 
 * Creates handler thread and returns its thread ID. 
 * @param left_addr 7-bit i2c address of left-side motor controller
 * @param right_addr 7 bit i2c address of right side motor controller
 * @param dev zephyr i2c device that each motor driver is run on. 
 * @param motor_info uninitialized struct (it is initialized in this function). 
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
    
    k_msgq_init(
        &(motor_info->handler_queue), 
        motor_info->cmd_msg_buf, 
        sizeof(struct rover_queue_msg), 
        16
        );

    // Create thread and pass in handler struct to avoid global. 
    LOG_INF("Creating thread to handle rover motor control");
    return k_thread_create(&(motor_info->motor_thread_data), rovermotor_handler_stack,
		K_THREAD_STACK_SIZEOF(rovermotor_handler_stack),
		rovermotor_handler,
		(void*) (motor_info), NULL, NULL,
		ROVERMOTOR_PRIORITY, 0, K_NO_WAIT);
}

/**
 * Hanlder thread for rover motor control. Blocks until a message is received from 
 * another task (from call to rovermotor_send_instruction). 
 * First parameter is pointer to handler struct rovermotor_info
 */
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
    while (1) {
        msg_rx_status = k_msgq_get(&(info.handler_queue), (void*) (&instruction), K_FOREVER);
        if (msg_rx_status) {
            LOG_INF("Couldn't get command from queue, %i", msg_rx_status); 
        } else {
            // We have a command to instruct the motor driver. 
            // Set angular velocity - scale -100-100% (0 to 1) command
            int left = instruction.direction * ANGLE_CHANGE_RATE_CONST; 
            // Turning only - left will move at opposite speed to right
            int right = left * -1; 

            // Adjust for forward velocity - increase vel of each motor to retain turning.
            left += ((int) (instruction.velocity)) * FWD_VELOCITY_RATE_CONST;
            right += ((int) (instruction.velocity)) * FWD_VELOCITY_RATE_CONST; 
            LOG_INF("Left: %i, right: %i", left, right);

            // avoid wraparound when casting. 
            if (left > 127) {
                left = 127; 
            } else if (left < -128) {
                left = -128; 
            }

            if (right > 127) {
                right = 127; 
            } else if (right < -128) {
                right = -128; 
            }
            // Map int8 range to 0-255 for motor drive. 
            uint8_t left_motor_cmd = left + 127; 
            uint8_t right_motor_cmd = right + 127; 
            
            // Control each motor. 
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
    // Send across to thread. Don't wait to avoid blocking caller task. 
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

