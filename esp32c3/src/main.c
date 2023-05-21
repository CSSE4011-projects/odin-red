
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include "rovermotor.h"
#include "roveruart.h"
//#include "pos_mobile_bt.h"
#define LOG_LEVEL 4
LOG_MODULE_REGISTER(main);

//  Global to pass to shell commands. 
struct rovermotor_info motor_control_handle; 
struct roveruart_info uart_control_handle; 
const struct device* i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
const struct device* uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

static int rover_cmd_cb(const struct shell* shell, size_t argc, char** argv) 
{
	int8_t vel = atoi(argv[1]); 
	int angle_percent = atoi(argv[2]); 
	float angle = ((float) (angle_percent)) / 100; 
	LOG_INF("Commanding speed: %hhi, angle: %f", vel, angle);
	rovermotor_send_instruction(&motor_control_handle, angle, vel);
	return 0;
}


static int reset_angle_cmd_cb(const struct shell* shell, size_t argc, char** argv) 
{
	roveruart_reset_angle(&uart_control_handle);
	LOG_INF("Resetting angle");
	return 0;
}
#define STACKSIZE 1024
#define PRIORITY 7
#define MSG_SIZE 32
//K_THREAD_DEFINE(ble_id, STACKSIZE * 4, ble_connect_main, NULL, NULL, NULL,
//		PRIORITY, 0, 0);

int main(void) 
{
    //int err = bt_enable(NULL);

	rovermotor_init(0x5D, 0x5E, i2c_dev, &motor_control_handle); 
	roveruart_init(uart_dev, &uart_control_handle); 
	// LOG_INF("Initialized motor handler");

	SHELL_CMD_ARG_REGISTER(
        rover,
        NULL,
        "rover [speed] [angle]: set speed and angle \n"
		"speed from -127 to 127, angle from -100 to 100 (int)",
        rover_cmd_cb,
        3,
        0
    );
	SHELL_CMD_ARG_REGISTER(
        reset_angle,
        NULL,
        "Reset angle refernce\n",
		reset_angle_cmd_cb,
        0,
        0
    );
	rover_position_info_t position;
	
 
	while (1)
	{
		// Do nothing - shell command handles it all. 
		k_sleep(K_MSEC(1000));
		int res = roveruart_get_new_position(&uart_control_handle, &position);
		if (res) {
			//LOG_INF("No new messages");
		} else {
			LOG_INF("Got position: (%hhu, %hhu)", position.x, position.y); 
		}
	}
	
	
}


