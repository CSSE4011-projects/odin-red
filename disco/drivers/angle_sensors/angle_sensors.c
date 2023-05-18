
#include "angle_sensors.h"

/* Macros for Kalman filter weights */
#define GYRO_WEIGHT     1
#define MAGN_WEIGHT     (1 - GYRO_WEIGHT)

/* Calibration constants */
#define MAGN_X_MAX      0.08
#define MAGN_X_MIN      -0.33
#define MAGN_Y_MAX      0.22
#define MAGN_Y_MIN      -0.14
// #define MAGN_X_MAX      5.24
// #define MAGN_X_MIN      3.16
// #define MAGN_Y_MAX      2.25
// #define MAGN_Y_MIN      -0.04
#define GYRO_Z_OFFSET   0.8
#define MAGN_X_SCALER   ((MAGN_X_MAX - MAGN_X_MIN) / 2)
#define MAGN_X_OFFSET   (MAGN_X_SCALER - MAGN_X_MAX)
#define MAGN_Y_SCALER   ((MAGN_Y_MAX - MAGN_Y_MIN) / 2)
#define MAGN_Y_OFFSET   (MAGN_Y_SCALER - MAGN_Y_MAX)

/* Initialising message queue for sending angle data */
K_MSGQ_DEFINE(
    angle_msgq,
    sizeof(struct angle_data),
    ANGLE_MSGQ_MAX_MSG,
    ANGLE_MSGQ_ALIGN
);

/* Semaphore to signal refernce angle updates */
K_SEM_DEFINE(update_ref_angle_sem, 0, 1);

/* Macro definitions for angle sensors message queue */
void angle_sensors_thread(void *, void *, void *)
{
    /* Get pointers to sensor devices */
	const struct device *const lis3mdl_dev = DEVICE_DT_GET_ONE(st_lis3mdl_magn);
	const struct device *const lsm6dsl_dev = DEVICE_DT_GET_ONE(st_lsm6dsl);

	struct sensor_value magn_x_raw, magn_y_raw, gyro_z_raw;

	if (!device_is_ready(lsm6dsl_dev)) {
		printf("Device lsm6dsl is not ready\n");
		return;
	}

    if (!device_is_ready(lis3mdl_dev)) {
		printf("Device lis3mdl is not ready\n");
		return;
	}

    /* Used to set attributes for the lsm6dsl sensor */
    struct sensor_value lsm6dsl_attr;

	/* Set accel/gyro sampling frequency to 104 Hz */
	lsm6dsl_attr.val1 = 104;
	lsm6dsl_attr.val2 = 0;

	if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &lsm6dsl_attr) < 0) {
		printf("Cannot set sampling frequency for gyro.\n");
		return;
	}

    /* Variables to store sensor values and angle calculations */
    double magn_x, magn_y, gyro_z;
    double angle, prev_angle;

    /* Struct to store all sensor values to be sent in message queue */
    struct angle_data angle_info;

    /* Set initial angle based on magnetometer readings */
    sensor_sample_fetch_chan(lis3mdl_dev, SENSOR_CHAN_MAGN_XYZ);
    sensor_channel_get(lis3mdl_dev, SENSOR_CHAN_MAGN_X, &magn_x_raw);
    sensor_channel_get(lis3mdl_dev, SENSOR_CHAN_MAGN_Y, &magn_y_raw);

    magn_x = (sensor_value_to_double(&magn_x_raw) + MAGN_X_OFFSET) / MAGN_X_SCALER;
    magn_y = (sensor_value_to_double(&magn_y_raw) + MAGN_Y_OFFSET) / MAGN_Y_SCALER;

    prev_angle = atan2(magn_y, magn_x) * 57.3;
    if (prev_angle < 0) {
        prev_angle += 360;
    }

    /* Initialise reference angle to 0 degrees */
    double reference_angle = 0;

    while (1) {
        /* Check if update reference angle semaphore has been given */
        if (!k_sem_take(&update_ref_angle_sem, K_NO_WAIT)) {
            reference_angle = prev_angle;
        }

		/* Fetch LSM6DSL Gyroscope values */
		sensor_sample_fetch_chan(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ);
        sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_GYRO_Z, &gyro_z_raw);

		/* Fetch LIS3MDL Magnetometer values */
		sensor_sample_fetch_chan(lis3mdl_dev, SENSOR_CHAN_MAGN_XYZ);
		sensor_channel_get(lis3mdl_dev, SENSOR_CHAN_MAGN_X, &magn_x_raw);
		sensor_channel_get(lis3mdl_dev, SENSOR_CHAN_MAGN_Y, &magn_y_raw);

        /* Calibrate sensor values */
        gyro_z = (sensor_value_to_double(&gyro_z_raw) * 57.3) - GYRO_Z_OFFSET;
	    magn_x = (sensor_value_to_double(&magn_x_raw) + MAGN_X_OFFSET) / MAGN_X_SCALER;
        magn_y = (sensor_value_to_double(&magn_y_raw) + MAGN_Y_OFFSET) / MAGN_Y_SCALER;

        /* Convert magnetometer readings to an angle */
        angle = atan2(magn_y, magn_x) * 57.3;
        if (angle < 0) {
            angle += 360;
        }
	    angle = 360 - angle;

        /* Use sensor fusion to update the angle based on a linear
            combination between the new angle and the old angle
            combined with the angular velocity */
	    prev_angle = GYRO_WEIGHT * (prev_angle + (gyro_z/5)) + MAGN_WEIGHT * angle;
		if (prev_angle < 0) {
			prev_angle += 360;
		} else if (prev_angle >= 360) {
			prev_angle -= 360;
		}

        /* Send calculated angle using msgq */
        angle_info.angle = prev_angle - reference_angle;
        if (angle_info.angle < 0) {
            angle_info.angle += 360;
        }

        /* Send angle data to message queue */
        if (k_msgq_put(&angle_msgq, &angle_info, K_NO_WAIT) != 0) {
            /* Queue is full, purge it */
            k_msgq_purge(&angle_msgq);
        }

        // TODO: change this to a timestamping approach to keep values sent
        // at consistent rate
        k_msleep(200);
    }
}