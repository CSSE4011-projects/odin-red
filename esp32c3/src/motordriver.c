#include "motordriver.h"
LOG_MODULE_REGISTER(motordriver);

/**
 * initializes an instance of a driver for the Sparkfun Qwiic Motor Driver /
 * Sparkfun SCMD motor driver, on given uart device, with given device address
 * @param dev: zephyr i2c device
 * @param device_addr: i2c address of qwiic motor driver. 
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
        // Keep trying - it might come online. 
        k_sleep(K_MSEC(100));
        status = i2c_reg_read_byte(dev, device_addr, SCMD_ID, &res);
        if (num_tries++ > CONN_MAX_NUM_TRIES) {
            LOG_ERR("Couldn't find driver with addr %x", device_addr);
            return; 
        };

    }
    if (status) {
        LOG_ERR("Dummy read failed");
        // The driver did not come online - ignore for now. 
    } else {
        LOG_INF("Found motor driver with id %x", res);
    }

    // Enable, check enable byte. Bridge left and right channels. 
    i2c_reg_write_byte(dev, device_addr, SCMD_DRIVER_ENABLE, 0x01);

    i2c_reg_read_byte(dev, device_addr, SCMD_DRIVER_ENABLE, &res);
    i2c_reg_write_byte(dev, device_addr, SCMD_BRIDGE, 0x01);
    
    // Zero the motors (in case the driver was left powere)
    motordriver_send_pwm(dev, device_addr, ZERO_SIGNAL); 
}


/** 
 * Sends a value for motor pwm to the motor controller with given i2c addr
 * @param dev: zephyr i2c device
 * @param device_addr: qwiic motor driver i2c address
 * @param val: value to send.
 */
int motordriver_send_pwm(const struct device* dev, uint8_t device_addr, uint8_t val)
{
    //LOG_INF("Motors: %hhu", val);
    int res = i2c_reg_write_byte(dev, device_addr, SCMD_MA_DRIVE, val);
    if (res) {
        return res;
    }
    res = i2c_reg_write_byte(dev, device_addr, SCMD_MB_DRIVE, val);
    return res;
    
}
