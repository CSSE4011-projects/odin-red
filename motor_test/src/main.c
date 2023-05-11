
#include "motordriver.h"
#include <stdlib.h>
#include <zephyr/zephyr.h>

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#define LOG_LEVEL 4
LOG_MODULE_REGISTER(main);




const uint8_t cts_sample_devids[] = {
	13,
	14,
	15,
	17
};
#define NUM_DEVIDS_CTS_SAMPLE 4

void main(void) {

	//shell_cmd_init();
	
	const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
	uint8_t addr = 0x5D; 
	motordriver_init(i2c_dev, addr); 
	while (1)
	{
		motordriver_send_pwm(i2c_dev, addr, 100, 1);

		k_sleep(K_MSEC(1234));
		motordriver_send_pwm(i2c_dev, addr, 10, 1);
		k_sleep(K_MSEC(4321));
	}
	
	
}
