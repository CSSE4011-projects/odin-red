#include "motordriver.h"
LOG_MODULE_REGISTER(motordriver);

void motordriver_init(const struct device* dev, uint8_t device_addr) 
{
    if (!device_is_ready(dev)) {
		LOG_ERR("I2C: Device is not ready.\n");
		return;
	}
    return; 
}

int motordriver_send_pwm(const struct device* dev, uint8_t device_addr, uint8_t val, uint8_t is_left)
{


	//struct i2c_msg msgs[2];

	/* Send the address to write to */
    /*
    uint8_t msg_address = SCMD_MA_DRIVE + is_left;
	msgs[0].buf = &msg_address;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;
    */ 
	/* Data to be written, and STOP after this. */
    /*
	msgs[1].buf = &val;
	msgs[1].len = 1;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, &msgs[0], 2, device_addr);

    */
    // Might also work:
    
    uint8_t buf[] = {SCMD_MA_DRIVE + is_left, val};
    return i2c_write(dev, &buf[0], sizeof(buf), device_addr);
    
    
    
}


int motordriver_read_pwm(const struct device* dev, uint8_t device_addr, uint8_t val, uint8_t is_left) 
{ 
    uint8_t buf[] = {SCMD_MA_DRIVE + is_left}; 
    int res = i2c_read(dev, &buf[0], sizeof(buf), device_addr);
    return res;
}