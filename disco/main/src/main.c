
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include "distance_sensors/distance_sensors.h"
#include "angle_sensors/angle_sensors.h"

/* Thread to handle calculating angle using magnetometer and gyroscope sensors */
K_THREAD_STACK_DEFINE(angle_thread_stack, ANGLE_THREAD_STACK_SIZE);
struct k_thread angle_thread_data;

/* Thread to handle measuring distance using VL53L1X TOF sensors */
K_THREAD_STACK_DEFINE(distance_thread_stack, DISTANCE_THREAD_STACK_SIZE);
struct k_thread distance_thread_data;

/* Function definitions */
void find_solution_lines(double distance, double angle, uint8_t* pos_buffer);
void predict_location(double angle, double front_d, double back_d, double left_d, double right_d, uint8_t* pos_buffer);

void main(void)
{
    /* Create a thread to handle angle sensors */
	k_tid_t angle_thread_id = k_thread_create(&angle_thread_data,
													angle_thread_stack,
													ANGLE_THREAD_STACK_SIZE,
													angle_sensors_thread,
													NULL, NULL, NULL,
													ANGLE_THREAD_PRIORITY,
													0, K_NO_WAIT);

    /* Create a thread to handle distance sensors */
	k_tid_t distance_thread_id = k_thread_create(&distance_thread_data,
													distance_thread_stack,
													DISTANCE_THREAD_STACK_SIZE,
													distance_sensors_thread,
													NULL, NULL, NULL,
													DISTANCE_THREAD_PRIORITY,
													0, K_NO_WAIT);

    /* Start angle sensors thread */
	if (angle_thread_id != NULL) {
		k_thread_start(angle_thread_id);
	}

	/* Start distance sensors thread */
	if (distance_thread_id != NULL) {
		k_thread_start(distance_thread_id);
	}

	/* Struct to store angle data received from angle sensors thread */
	struct angle_data angle;

	/* Struct to store distances data received from distance sensors thread */
	struct distance_data distances;

    /* Buffer to store the predicted x,y location of the rover */
    uint8_t pred_location[2];

    while (1) {

        // TODO: check for update reference angle from UART thread here

		/* Check for angle data */
		k_msgq_get(&angle_msgq, &angle, K_NO_WAIT);

		/* Check for distances data */
		k_msgq_get(&distances_msgq, &distances, K_NO_WAIT);

        predict_location(angle.angle,
                distances.front_distance,
                distances.back_distance,
                distances.left_distance,
                distances.right_distance,
                pred_location);

        // TODO: send pred location to uart thread
        
        // TODO: change this to timestamping
		k_sleep(K_MSEC(200));        
    }
}

/* Takes in angle and four distances (front, back, left and right) and
    predicts the location using trigonometry. Places the predicted x,y
    location in the supplied buffer.
    Note: angle must be in degrees and between 0 and 360. 
    */
void predict_location(double angle, double front_d, double back_d, double left_d, double right_d, uint8_t* pos_buffer)
{
    uint8_t x, y;

    double front_angle, back_angle, left_angle, right_angle;
    front_angle = angle * 0.01744; // Convert to radians
    back_angle = ((angle + 180) % 360) * 0.01744
    left_angle = ((angle + 90) % 360) * 0.01744
    right_angle = ((angle + 270) % 360) * 0.01744

    // TODO: finish implementing trig here

    pos_buffer[0] = x;
    pos_buffer[1] = y;
}

/* Helper function to find the two solution lines (y=a and x=b) where
    the rover could be based on a given distance to a wall and an angle.
    Stores the x,y functions in the given buffer. */
void find_solution_lines(double distance, double angle, uint8_t* pos_buffer)
{
    // TODO: finish implementing this helper function
}