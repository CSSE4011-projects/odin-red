# Changes to Zephyr driver for LIS3MDL
Errors found in the existing driver:
* The driver appears to support both SINGLE and CONTINUOUS sampling modes (can select these control bits) but does not do this correctly.
* The driver sets SINGLE mode for FAST sampling modes and CONTINUOUS for regular sampling modes (inverted to the datasheet).

In SINGLE sampling mode, the LIS3MDL returns to IDLE mode after each sample (i.e., each time a sample is wanted from the sensor, the sensor must be put back into SINGLE mode to trigger a new measurement). The driver however, does not place the sensor back into SINGLE mode and instead will constantly return the first sensor reading (from startup) each time sensor fetch and sensor get is called. This has been corrected by checking if the sensor is in single mode on each sample fetch, and setting the relevant control bit to trigger a new measurement.

Currently, the driver has a turnery statement to select SINGLE mode for FAST sampling modes and CONTINUOUS for regular sampling modes. In the datasheet for the LIS3MDL, it specifies the opposite of this (sampling modes without the fast ODR bit set must be used with SINGLE sampling mode, otherwise CONTINUOUS). This has been corrected in the updated driver.

### When building this project, ensure that the Zephyr file structure has the files included within this folder instead of the existing files.
The files can be found at: `zephyrproject/zephyr/drivers/sensor/lis3mdl`