#include "motordriver.h"
LOG_MODULE_REGISTER(motordriver);

void motordriver_init(const struct device* dev, uint8_t device_addr) 
{

    if (!device_is_ready(dev)) {
		LOG_ERR("I2C: Device is not ready.\n");
		return;
	}
    // dummy read: 
    uint8_t res;
    int status = i2c_reg_read_byte(dev, device_addr, SCMD_ID, &res);
    int num_tries = 0; 
    while (res != 0xA9) 
    {
        LOG_INF("Couldn't get valid ID yet ... ");

        k_sleep(K_MSEC(100));
        status = i2c_reg_read_byte(dev, device_addr, SCMD_ID, &res);
        if (num_tries++ > 10) {
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
    return; 
}

int motordriver_send_pwm(const struct device* dev, uint8_t device_addr, uint8_t val)
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
    
    //uint8_t buf[] = {SCMD_MA_DRIVE + is_left, val};

    uint8_t buf = val;
    //uint16_t data_addr = device_addr << 8 || (SCMD_MA_DRIVE + is_left);  
    uint16_t data_addr = 0x5D20;
    //int res = i2c_write(dev, &buf, sizeof(buf), data_addr);
    //uint8_t rd_buf[2]; 
    //int read_res = i2c_read(dev, &rd_buf[0], 2, device_addr); 


    int res = i2c_reg_write_byte(dev, device_addr, SCMD_MA_DRIVE, val);
    if (res) {
        return res;
    }
    res = i2c_reg_write_byte(dev, device_addr, SCMD_MB_DRIVE, val);
    return res;
    
}
