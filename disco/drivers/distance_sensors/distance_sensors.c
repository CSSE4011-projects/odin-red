
#include "distance_sensors.h"

/* Initialising message queue for sending angle data */
K_MSGQ_DEFINE(
    distances_msgq,
    sizeof(struct distance_data),
    DISTANCE_MSGQ_MAX_MSG,
    DISTANCE_MSGQ_ALIGN
);

#define SAMPLE_TIME_MS 		500
#define SENSOR_NO_RESPONSE 	-1

/* Entry point to distance sensors thread */
void distance_sensors_thread(void *, void *, void *)
{
	// Get vl53l1x device configurations from device tree
	const struct device *const vl53l1x_front_dev = DEVICE_DT_GET(DT_NODELABEL(vl53l1x_front));
	const struct device *const vl53l1x_back_dev = DEVICE_DT_GET(DT_NODELABEL(vl53l1x_back));
	const struct device *const vl53l1x_left_dev = DEVICE_DT_GET(DT_NODELABEL(vl53l1x_left));
	const struct device *const vl53l1x_right_dev = DEVICE_DT_GET(DT_NODELABEL(vl53l1x_right));

    /* Variables to store sensor values */
	struct sensor_value front_value, back_value, left_value, right_value;
	double front_d_double, back_d_double, left_d_double, right_d_double;

    /* Struct for sending distances through msgq */
    struct distance_data distances;

	// Ensure sensors are ready
	if (!device_is_ready(vl53l1x_front_dev)
            || !device_is_ready(vl53l1x_back_dev)
            || !device_is_ready(vl53l1x_left_dev)
            || !device_is_ready(vl53l1x_right_dev)) {

		printk("vl53l1x sensors not ready.\n");
		return;
	}

    /* Used to set attributes for the vl53l1x sensor */
    struct sensor_value vl53l1x_attr;

	/* Set distance mode to medium */
	vl53l1x_attr.val1 = 1;
	vl53l1x_attr.val2 = 0;

	if (sensor_attr_set(vl53l1x_front_dev, SENSOR_CHAN_DISTANCE,
			    SENSOR_ATTR_CONFIGURATION, &vl53l1x_attr) < 0) {
		printf("Cannot set distance mode for front vl53l1x.\n");
		return;
	}

	if (sensor_attr_set(vl53l1x_back_dev, SENSOR_CHAN_DISTANCE,
			    SENSOR_ATTR_CONFIGURATION, &vl53l1x_attr) < 0) {
		printf("Cannot set distance mode for back vl53l1x.\n");
		return;
	}
	
	if (sensor_attr_set(vl53l1x_left_dev, SENSOR_CHAN_DISTANCE,
			    SENSOR_ATTR_CONFIGURATION, &vl53l1x_attr) < 0) {
		printf("Cannot set distance mode for left vl53l1x.\n");
		return;
	}
	
	if (sensor_attr_set(vl53l1x_right_dev, SENSOR_CHAN_DISTANCE,
			    SENSOR_ATTR_CONFIGURATION, &vl53l1x_attr) < 0) {
		printf("Cannot set distance mode for right vl53l1x.\n");
		return;
	}

    /* Fetch sensors one by one to initialise and
        reconfigure I2C addresses */
    sensor_sample_fetch(vl53l1x_front_dev);
    sensor_sample_fetch(vl53l1x_back_dev);
    sensor_sample_fetch(vl53l1x_left_dev);
    sensor_sample_fetch(vl53l1x_right_dev);

	uint32_t prev_time = k_uptime_get_32();
	uint32_t current_time;

    while (1) {

        /* Trigger sensors to read new values and get new measurements */
        sensor_sample_fetch(vl53l1x_front_dev);
		sensor_channel_get(vl53l1x_front_dev, SENSOR_CHAN_DISTANCE, &front_value);

        sensor_sample_fetch(vl53l1x_back_dev);
		sensor_channel_get(vl53l1x_back_dev, SENSOR_CHAN_DISTANCE, &back_value);

        sensor_sample_fetch(vl53l1x_left_dev);
		sensor_channel_get(vl53l1x_left_dev, SENSOR_CHAN_DISTANCE, &left_value);

        sensor_sample_fetch(vl53l1x_right_dev);
		sensor_channel_get(vl53l1x_right_dev, SENSOR_CHAN_DISTANCE, &right_value);

		front_d_double = sensor_value_to_double(&front_value);
		back_d_double = sensor_value_to_double(&back_value);
		left_d_double = sensor_value_to_double(&left_value);
		right_d_double = sensor_value_to_double(&right_value);

		if (front_d_double != SENSOR_NO_RESPONSE) {
			distances.front_distance = front_d_double;
		}

		if (back_d_double != SENSOR_NO_RESPONSE) {
			distances.back_distance = back_d_double;
		}

		if (left_d_double != SENSOR_NO_RESPONSE) {
			distances.left_distance = left_d_double;
		}

		if (right_d_double != SENSOR_NO_RESPONSE) {
			distances.right_distance = right_d_double;
		}

        /* Send distances data to message queue */
		current_time = k_uptime_get_32();
        if (current_time - prev_time > SAMPLE_TIME_MS) {
			if (k_msgq_put(&distances_msgq, &distances, K_NO_WAIT) != 0) {
				/* Queue is full, purge it */
				k_msgq_purge(&distances_msgq);
			}
			prev_time = current_time;
		}

		k_sleep(K_MSEC(10));
    }
}