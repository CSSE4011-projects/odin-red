
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include "rovermotor.h"
#include "roveruart.h"
#include "ahu_rgb.h"
#include <math.h> 
#include "pos_mobile_bt.h"
#define LOG_LEVEL 4
LOG_MODULE_REGISTER(main);

// RGB thread stack
K_THREAD_STACK_DEFINE(ahu_rgb_stack, AHU_RGB_STACK_SIZE);
struct k_thread ahu_rgb_thread_data;

//  Global to pass to shell commands. 
struct rovermotor_info motor_control_handle; 
struct roveruart_info uart_control_handle; 
const struct device* i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
const struct device* uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

// Helper function definition
void send_ahu_rgb(uint8_t red, uint8_t green, uint8_t blue);

static int rgb_cmd_callback(const struct shell* shell, size_t argc, char** argv)
{
	/* Struct to send rgb control information to the ahu rgb thread */
	struct ahu_rgb_colour colour;
	int r = atoi(argv[1]);
	int g = atoi(argv[2]);
	int b = atoi(argv[3]);

	if (r >= 0 && g >= 0 && b >= 0
			&& r < 256 && g < 256 && b < 256) {
		shell_fprintf(shell, SHELL_NORMAL,
				"writing rgb <%d,%d,%d>\n", r, g, b);
		send_ahu_rgb(r, g, b);
	} else {
		shell_fprintf(shell, SHELL_ERROR,
				"invalid rgb, 0 <= rgb values <= 255\n");
	}

	return 0;
}

static int rover_cmd_cb(const struct shell* shell, size_t argc, char** argv) 
{
	int8_t vel = atoi(argv[1]); 
	int angle_percent = atoi(argv[2]); 
	float angle = ((float) (angle_percent)) / 100; 
	LOG_INF("Commanding speed: %hhi, angle: %f", vel, angle);
	rovermotor_send_instruction(&motor_control_handle, angle, vel);
	int8_t r = vel + 127; 
	int8_t b = (255 - r);
	int8_t g = (int8_t) (angle * 128) + 127; 
	send_ahu_rgb(r, g, b); 
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

	/* Create a thread to control the ahu rgb led */
	k_tid_t ahu_rgb_thread_id = k_thread_create(&ahu_rgb_thread_data,
													ahu_rgb_stack,
													AHU_RGB_STACK_SIZE,
													ahu_rgb_thread,
													NULL, NULL, NULL,
													AHU_RGB_PRIORITY,
													0, K_NO_WAIT);

	rovermotor_init(0x5D, 0x5E, i2c_dev, &motor_control_handle); 
	roveruart_init(uart_dev, &uart_control_handle); 
	// LOG_INF("Initialized motor handler");

	SHELL_CMD_ARG_REGISTER(
		rgb,
		NULL,
		"rgb <r> <g> <b>",
		rgb_cmd_callback,
		4,
		0
	);

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

	/* Struct to send rgb control information to the ahu rgb thread */
	struct ahu_rgb_colour rgb_colour;

	/* Struct to send position data to the bt thread */
	struct pos_data position_bt;

	/* Struct to receive control data from the bt thread */
	struct control_data control;
	
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

		/* Check for control data from bt thread */
		if (!k_msgq_get(&control_msgq, &control, K_NO_WAIT)) {
			// LACHLAN do something with the rover driver
			// THESE ARE THE VALUES :)
			// control.pedal_left;
			// control.pedal_right;
			// control.rudder_angle;
		}

		/* LACHLAN I THINK THIS IS RIGHT BUT CAN YOU CHECK */
		position_bt.x_pos = position.x;
		position_bt.y_pos = position.y;

		/* Send position data to bt thread */
		if (k_msgq_put(&pos_msgq, &position_bt, K_NO_WAIT) != 0) {
			/* Queue is full, purge it */
			k_msgq_purge(&pos_msgq);
		}

	}
	
	
}

/* Send the given rgb value to the ahu rgb led */
void send_ahu_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
	/* Struct to send rgb control information to the ahu rgb thread */
	struct ahu_rgb_colour colour;
	colour.red = red;
	colour.green = green;
	colour.blue = blue;

	/* Send rgb control struct to rgb message queue */
	if (k_msgq_put(&ahu_rgb_msgq, &colour, K_NO_WAIT) != 0) {
		/* Queue is full, purge it */
		k_msgq_purge(&ahu_rgb_msgq);
	}
}

