# Project Overview
Control a small rover using microROS with a USB Foot Pedal interface. Localize positon using Time-of-flight (ToF) sensors, and track heading using magnetometer and Kalman filter. 


## KPIs
### 1. Control response time
Target less than one second for pedal input value change to effect noticeable change to vehicle speed or direction. 

### 2. Localization accuracy 

Rover localization should be accurate to 15cm in x and y directions, at all points on the grid. 

### 3. Heading accuracy

Rover heading (direction) relative to middle-top part of grid, is tracked continuously and accurate to 10 degrees. 

### 4. Web dashboard response time (latency)

Changing values for heading and position are updated within 2 seconds of value changing

### 5. Web dashboard refresh rate (throughput)
Web dashboard values update at rate of 1Hz during operation. 

### 6. Control 'feel'
Controls feel responsive, and vehicle moves at speeds proportional to pedal input level. Controls are accurate and precise. 