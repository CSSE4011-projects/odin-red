/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <math.h>

#define MAGN_X_MAX 0.08
#define MAGN_X_MIN -0.33
#define MAGN_Y_MAX 0.22
#define MAGN_Y_MIN -0.14
#define GYRO_Z_OFFSET 0.8

#define MAGN_X_SCALER ((MAGN_X_MAX - MAGN_X_MIN) / 2)
#define MAGN_X_OFFSET (MAGN_X_SCALER - MAGN_X_MAX)
#define MAGN_Y_SCALER ((MAGN_Y_MAX - MAGN_Y_MIN) / 2)
#define MAGN_Y_OFFSET (MAGN_Y_SCALER - MAGN_Y_MAX)

void main(void)
{
    /* Get pointers to sensor devices */
	const struct device *const lis3mdl_dev = DEVICE_DT_GET_ONE(st_lis3mdl_magn);
	const struct device *const lsm6dsl_dev = DEVICE_DT_GET_ONE(st_lsm6dsl);

    /* Defining sensor_value structs to store sensor data */
	// struct sensor_value accel_x, accel_y, accel_z;
	// struct sensor_value gyro_x, gyro_y, gyro_z;
	// struct sensor_value magn_x, magn_y, magn_z;
	struct sensor_value magn_x, magn_y, gyro_z;

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

	// if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ,
	// 		    SENSOR_ATTR_SAMPLING_FREQUENCY, &lsm6dsl_attr) < 0) {
	// 	printf("Cannot set sampling frequency for accelerometer.\n");
	// 	return;
	// }

	if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &lsm6dsl_attr) < 0) {
		printf("Cannot set sampling frequency for gyro.\n");
		return;
	}

	double prev_gyro;
	double gz;

	sensor_sample_fetch_chan(lis3mdl_dev, SENSOR_CHAN_MAGN_XYZ);
	sensor_channel_get(lis3mdl_dev, SENSOR_CHAN_MAGN_X, &magn_x);
	sensor_channel_get(lis3mdl_dev, SENSOR_CHAN_MAGN_Y, &magn_y);

	double mx = sensor_value_to_double(&magn_x);
	double my = sensor_value_to_double(&magn_y);

	mx = (mx + MAGN_X_OFFSET) / MAGN_X_SCALER;
	my = (my + MAGN_Y_OFFSET) / MAGN_Y_SCALER;

	double angle = atan2(my, mx) * 57.3;
	if (angle < 0) {
		angle += 360;
	}

	angle = 360 - angle;
	prev_gyro = angle;

    while (1) {

		//k_sleep(K_SECONDS(1));
		k_msleep(200);

		// /* LSM6DSL Acceleration */
		// sensor_sample_fetch_chan(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ);
		// sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_ACCEL_X, &accel_x);
		// sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_ACCEL_Y, &accel_y);
		// sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_ACCEL_Z, &accel_z);

		// /* LSM6DSL Gyroscope */
		sensor_sample_fetch_chan(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ);
		// sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_GYRO_X, &gyro_x);
		// sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_GYRO_Y, &gyro_y);
		sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_GYRO_Z, &gyro_z);

		/* LIS3MDL Magnetometer */
		sensor_sample_fetch_chan(lis3mdl_dev, SENSOR_CHAN_MAGN_XYZ);
		sensor_channel_get(lis3mdl_dev, SENSOR_CHAN_MAGN_X, &magn_x);
		sensor_channel_get(lis3mdl_dev, SENSOR_CHAN_MAGN_Y, &magn_y);
		// sensor_channel_get(lis3mdl_dev, SENSOR_CHAN_MAGN_Z, &magn_z);

        // double ax = sensor_value_to_double(&accel_x); // m/s^2
        // double ay = sensor_value_to_double(&accel_y);
        // double az = sensor_value_to_double(&accel_z);

	    // double gx = sensor_value_to_double(&gyro_x);
        // double gy = sensor_value_to_double(&gyro_y);
        gz = (sensor_value_to_double(&gyro_z) * 57.3) - GYRO_Z_OFFSET;

	    mx = sensor_value_to_double(&magn_x);
        my = sensor_value_to_double(&magn_y);
        // double mz = sensor_value_to_double(&magn_z);

		// printf("accel x:%f ms/2 y:%f ms/2 z:%f ms/2\n", ax, ay, az);
		// printf("gyro x:%f  y:%f  z:%f \n", gx, gy, gz);
		// printf("magn x:%f  y:%f  z:%f \n", mx, my, mz);
		// printf("gyro: %f\n", gz);

		mx = (mx + MAGN_X_OFFSET) / MAGN_X_SCALER;
		my = (my + MAGN_Y_OFFSET) / MAGN_Y_SCALER;

		angle = atan2(my, mx) * 57.3;
		if (angle < 0) {
			angle += 360;
		}

		angle = 360 - angle;

		prev_gyro = 0.2 * (prev_gyro + gz) + 0.8 * angle;
		if (prev_gyro < 0) {
			prev_gyro += 360;
		} else if (prev_gyro >= 360) {
			prev_gyro -= 360;
		}

		printf("filtered angle: %f deg\n", prev_gyro);
		printf("angle: %f deg\n", angle);

    }
}
