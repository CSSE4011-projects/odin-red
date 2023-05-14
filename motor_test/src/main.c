
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/i2c.h>

#include "rovermotor.h"
#define LOG_LEVEL 4
LOG_MODULE_REGISTER(main);

//  Global to pass to shell commands. 
struct rovermotor_info motor_control_handle; 

const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

static int rover_cmd_cb(const struct shell* shell, size_t argc, char** argv) 
{
	int8_t vel = atoi(argv[1]); 
	int angle_percent = atoi(argv[2]); 
	float angle = ((float) (angle_percent)) / 100; 
	LOG_INF("Commanding speed: %hhi, angle: %f", vel, angle);
	rovermotor_send_instruction(&motor_control_handle, angle, vel);
	return 0;
}

void main(void) 
{

	rovermotor_init(0x5D, 0xAB, i2c_dev, &motor_control_handle); 
	LOG_INF("Initialized motor handler");

	SHELL_CMD_ARG_REGISTER(
        rover,
        NULL,
        "rover [speed] [angle]: set speed and angle \n"
		"speed from -127 to 127, angle from -100 to 100 (int)",
        rover_cmd_cb,
        3,
        0
    );

	while (1)
	{
		// Do nothing - shell command handles it all. 
		k_sleep(K_MSEC(100));
		
	}
	
	
}


