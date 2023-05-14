#include "motordriver.h"
LOG_MODULE_REGISTER(motordriver);

/**
 * initializes an instance of a driver for the Sparkfun Qwiic Motor Driver /
 * Sparkfun SCMD motor driver, on given uart device, with given device address
 */
void motordriver_init(const struct device* dev, uint8_t device_addr) 
{

    if (!device_is_ready(dev)) {
		LOG_ERR("I2C: Device is not ready.\n");
		return;
	}
    // dummy read - check the device we expect is on this bus. 
    uint8_t res;
    int status = i2c_reg_read_byte(dev, device_addr, SCMD_ID, &res);
    int num_tries = 0; 
    while (res != ID_WORD) 
    {
        LOG_INF("Couldn't get valid ID yet ... ");

        k_sleep(K_MSEC(100));
        status = i2c_reg_read_byte(dev, device_addr, SCMD_ID, &res);
        if (num_tries++ > CONN_MAX_NUM_TRIES) {
            LOG_ERR("Couldn't find driver with addr %x", device_addr);
            return; 
        };

    }
    if (status) {
        LOG_ERR("Dummy read failed");
    } else {
        LOG_INF("Found motor driver with id %x", res);
    }
    i2c_reg_write_byte(dev, device_addr, SCMD_DRIVER_ENABLE, 0x01);

    i2c_reg_read_byte(dev, device_addr, SCMD_DRIVER_ENABLE, &res);
    i2c_reg_write_byte(dev, device_addr, SCMD_BRIDGE, 0x01);
}

int motordriver_send_pwm(const struct device* dev, uint8_t device_addr, uint8_t val)
{
    int res = i2c_reg_write_byte(dev, device_addr, SCMD_MA_DRIVE, val);
    if (res) {
        return res;
    }
    res = i2c_reg_write_byte(dev, device_addr, SCMD_MB_DRIVE, val);
    return res;
    
}
