
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/sys/util.h>
#include <math.h>
#include "distance_sensors/distance_sensors.h"
#include "angle_sensors/angle_sensors.h"
#include "serial_comms/serial_comms.h"

/* Macro definitions for constants */
#define Y_MAX_CM            200
#define Y_MIN_CM            0
#define X_MAX_CM            200
#define X_MIN_CM            0

#define DISTANCE_ERR_THRES  5

#define X_IDX               0
#define Y_IDX               1

/* Thread to handle calculating angle using magnetometer and gyroscope sensors */
K_THREAD_STACK_DEFINE(angle_thread_stack, ANGLE_THREAD_STACK_SIZE);
struct k_thread angle_thread_data;

/* Thread to handle measuring distance using VL53L1X TOF sensors */
K_THREAD_STACK_DEFINE(distance_thread_stack, DISTANCE_THREAD_STACK_SIZE);
struct k_thread distance_thread_data;

/* Thread to handle serial communications over uart */
K_THREAD_STACK_DEFINE(serial_comms_thread_stack, SERIAL_COMMS_STACK_SIZE);
struct k_thread serial_comms_thread_data;

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

    /* Create a thread to handle serial communications */
	k_tid_t serial_comms_thread_id = k_thread_create(&serial_comms_thread_data,
													serial_comms_thread_stack,
													SERIAL_COMMS_STACK_SIZE,
													serial_comms_thread,
													NULL, NULL, NULL,
													SERIAL_COMMS_PRIORITY,
													0, K_NO_WAIT);

    /* Start angle sensors thread */
	if (angle_thread_id != NULL) {
		k_thread_start(angle_thread_id);
	}

	/* Start distance sensors thread */
	if (distance_thread_id != NULL) {
		k_thread_start(distance_thread_id);
	}

	/* Start serial comms thread */
	if (serial_comms_thread_id != NULL) {
		k_thread_start(serial_comms_thread_id);
	}


	/* Struct to store angle data received from angle sensors thread */
	struct angle_data angle;

	/* Struct to store distances data received from distance sensors thread */
	struct distance_data distances;

    /* Struct to store predicted location to send to serial comms thread */
    struct serial_comms_data outgoing_location;

    /* Buffer to store the predicted x,y location of the rover */
    uint8_t pred_location[2];

    while (1) {

        /* Check if update reference angle semaphore has been given from UART thread */
        if (!k_sem_take(&serial_ref_angle_sem, K_NO_WAIT)) {
            /* Give update reference angle semaphore to angle thread */
            k_sem_give(&update_ref_angle_sem);
        }

		/* Check for angle data */
		k_msgq_get(&angle_msgq, &angle, K_NO_WAIT);

		/* Check for distances data */
		k_msgq_get(&distances_msgq, &distances, K_NO_WAIT);

        predict_location(angle.angle,
                distances.front_distance / 10,
                distances.back_distance / 10,
                distances.left_distance / 10,
                distances.right_distance / 10,
                pred_location);

        /* Send position to serial comms message queue */
        if (k_msgq_put(&serial_comms_msgq, &outgoing_location, K_NO_WAIT) != 0) {
            /* Queue is full, purge it */
            k_msgq_purge(&serial_comms_msgq);
        }
        
        // TODO: change this to timestamping
		k_sleep(K_MSEC(200));        
    }
}

/* Takes in angle and four distances (front, back, left and right) and
    predicts the location using trigonometry. Places the predicted x,y
    location in the supplied buffer.
    Note: angle must be in degrees and between 0 and 360.
          distance must be in cm. 
    */
void predict_location(double angle, double front_d, double back_d, double left_d, double right_d, uint8_t* pos_buffer)
{
    /* Determine angles for all four distances */
    double front_angle, back_angle, left_angle, right_angle;

    front_angle = angle * 0.01744; // Convert to radians

    back_angle = angle + 180;
    if (back_angle >= 360) {
        back_angle -= 360;
    }
    back_angle *= 0.01744; // Convert to radians

    left_angle = angle + 90;
    if (left_angle >= 360) {
        left_angle -= 360;
    }
    left_angle *= 0.01744; // Convert to radians

    right_angle = angle + 270;
    if (right_angle >= 360) {
        right_angle -= 360;
    }
    right_angle *= 0.01744; // Convert to radians

    /* Buffers to store possible x and y solution lines */
    uint8_t front_x_y_buf[2], back_x_y_buf[2], left_x_y_buf[2], right_x_y_buf[2];

    /* Find possible x and y positions for rover based on each distance */
    find_solution_lines(front_d, front_angle, front_x_y_buf);
    find_solution_lines(back_d, back_angle, back_x_y_buf);
    find_solution_lines(left_d, left_angle, left_x_y_buf);
    find_solution_lines(right_d, right_angle, right_x_y_buf);

    /* Store the x solutions in an array and the y solutions in an array */
    uint8_t xs[] = {
        front_x_y_buf[X_IDX],
        back_x_y_buf[X_IDX],
        left_x_y_buf[X_IDX],
        right_x_y_buf[X_IDX]
    };

    uint8_t ys[] = {
        front_x_y_buf[Y_IDX],
        back_x_y_buf[Y_IDX],
        left_x_y_buf[Y_IDX],
        right_x_y_buf[Y_IDX]
    };

    uint8_t i, j, k, l;
    for (i = 0; i < 4; i++) {
        j = (i + 1) % 4;
        k = (i + 2) % 4;
        l = (i + 3) % 4;

        /* Check if any combination of three lines are similar
            (equal but with noise) */
        if (abs(xs[i] - xs[j]) < DISTANCE_ERR_THRES
                && abs(xs[i] - xs[k]) < DISTANCE_ERR_THRES) {
            // Average lines
            uint16_t avg = round((xs[i] + xs[j] + xs[k]) / 3.0);
            /* Ignore zero */
            if (abs(avg) >= DISTANCE_ERR_THRES
                    && abs(ys[l]) >= DISTANCE_ERR_THRES) {
                pos_buffer[X_IDX] = (uint8_t) avg;
                pos_buffer[Y_IDX] = ys[l];
                return;
            }
        }

        if (abs(ys[i] - ys[j]) < DISTANCE_ERR_THRES
                && abs(ys[i] - ys[k]) < DISTANCE_ERR_THRES) {
            // Average lines
            uint16_t avg = round((ys[i] + ys[j] + ys[k]) / 3.0);
            /* Ignore zero */
            if (abs(xs[l]) >= DISTANCE_ERR_THRES
                    && abs(avg) >= DISTANCE_ERR_THRES) {
                pos_buffer[X_IDX] = xs[l];
                pos_buffer[Y_IDX] = (uint8_t) avg;
                return;
            }
        }
    }

    /* Check if any combinations of two lines are similar
        (equal but with noise) */
    uint8_t num_indexes = 6;
    uint8_t indexes_1[] = {0, 0, 0, 1, 1, 2};
    uint8_t indexes_2[] = {1, 2, 3, 2, 3, 3};

    for (uint8_t idx = 0; idx < num_indexes; idx++) {
        i = indexes_1[idx];
        j = indexes_2[idx];
        k = indexes_1[num_indexes - idx - 1];
        l = indexes_2[num_indexes - idx - 1];
        if (abs(xs[i] - xs[j]) < DISTANCE_ERR_THRES
                && abs(ys[k] - ys[l]) < DISTANCE_ERR_THRES) {
            uint16_t x_avg = round((xs[i] + xs[j]) / 2.0);
            uint16_t y_avg = round((ys[k] + ys[l]) / 2.0);
            
            if (abs(x_avg) >= DISTANCE_ERR_THRES
                    && abs(y_avg) >= DISTANCE_ERR_THRES) {
                pos_buffer[X_IDX] = (uint8_t) x_avg;
                pos_buffer[Y_IDX] = (uint8_t) y_avg;
            }

        }
    }

    pos_buffer[X_IDX] = 0;
    pos_buffer[Y_IDX] = 0;
    return;
}

/* Helper function to find the two solution lines (y=a and x=b) where
    the rover could be based on a given distance to a wall and an angle.
    Stores the x,y functions in the given buffer.
    Note: angle should be given in radians (between 0 and 2pi) and distance
    should be given in cm. */
void find_solution_lines(double distance, double angle, uint8_t* pos_buffer)
{
    /* Calculate x and y components of the given distance and angle */
    double yd = distance * sin(angle);
    double xd = distance * cos(angle);

    double angle_deg = angle * 57.3;

    /* Check edge cases (0, 90, 180 or 270 degrees) */
    if (angle_deg == 0) {
        pos_buffer[X_IDX] = (uint8_t) round(X_MAX_CM - xd);
        pos_buffer[Y_IDX] = 0;
        return;
    } else if (angle_deg == 90) {
        pos_buffer[X_IDX] = 0;
        pos_buffer[Y_IDX] = (uint8_t) round(Y_MAX_CM - yd);
        return;
    } else if (angle_deg == 180) {
        pos_buffer[X_IDX] = (uint8_t) round(X_MIN_CM - xd);
        pos_buffer[Y_IDX] = 0;
        return;
    } else if (angle_deg == 270) {
        pos_buffer[X_IDX] = 0;
        pos_buffer[Y_IDX] = (uint8_t) round(Y_MIN_CM - yd);
        return;
    }

    /* Map the x and y components to x and y coordinates within the 2m square */
    if (0 < angle_deg && angle_deg < 180) {
        pos_buffer[Y_IDX] = (uint8_t) round(Y_MAX_CM - yd);
    } else {
        pos_buffer[Y_IDX] = (uint8_t) round(Y_MIN_CM - yd);
    }

    if (90 < angle_deg && angle_deg < 270) {
        pos_buffer[X_IDX] = (uint8_t) round(X_MAX_CM - xd);
    } else {
        pos_buffer[X_IDX] = (uint8_t) round(X_MIN_CM - xd);
    }
}