# Changes to Zephyr driver for VL53L1X
Errors found in the existing driver:
* The gpio pin interrupt configure is called in the sample fetch function (once per sample) instead of once in the interrupt initialisation function.
* The existing driver does not have support for changing I2C addresses for use with multiple chips.

In the existing Zephyr driver for the VL53L1X, the interrupt will correctly configure on the first sensor fetch (as it configures the interrupt for the first time), however on the second sensor fetch, the driver will throw an error and exit. This is because it attempts to call the gpio interrupt configure each time it fetches a sample, and if its already intialised then this will return an EBUSY error code.

The current Zephyr driver for the VL53L1X does not include any functionality to reconfigure the I2C addresses of the sensors, for use with multiple chips. This functionality is required for the project and therefore had to be added in. It is based on a similar setup included in the VL53L0X sensor driver. By setting the new CONFIG_VL53L1X_RECONFIGURE_ADDRESS option, sensors can be specified in the device tree with their intended new address, and the driver will set this up automatically.

### When building this project, ensure that the Zephyr file structure has the files included within this folder instead of the existing files.
The files can be found at: `zephyrproject/zephyr/drivers/sensor/vl53l1x`