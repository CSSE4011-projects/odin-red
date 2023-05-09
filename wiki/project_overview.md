# Project Overview
### Control of a small rover utilising a USB Foot Pedal interface. With instructions sent to the rover over bluetooth low energy (BLE). Localize positon using Time-of-flight (ToF) sensors, and track heading using magnetometer and Kalman filter. <br></br>

## **Equipment**
1. 1x Saitek flight rudder pedals
2. 1x Motor driver (TBC)
3. 1x Rover kit
4. ST Micro B-L475E-IOT1A Discovery Kit
5. 2x Nordic Semiconductor NRF52840 USB dongles
6. 1x ESP32-C3 Microcontroller
7. 4x Laser Time of Flight (ToF) sensors (TBC) <br></br>


## **Key Performance Indicators (KPIs)**
### 1. **Control response time**
Target less than one second for pedal input value change to effect noticeable change to vehicle speed or direction. 

### **2. Localization accuracy**
Rover localization should be accurate to 15cm in x and y directions, at all points on the grid. 

### **3.Heading accuracy**
Rover heading (direction) relative to middle-top part of grid, is tracked continuously and accurate to 10 degrees. 

### **4. Web dashboard response time (latency)**

 Changing values for heading and position are updated within 2 seconds of value changing

### **5. Web dashboard refresh rate (throughput)**  
Web dashboard values update at rate of 1Hz during operation. 

### **6. Control 'feel'**
Controls feel responsive, and vehicle moves at speeds proportional to pedal input level. Controls are accurate and precise. <br></br>

<!-- TODO - ADD SOME LIMITATIONS OF THE MODEL -->
## **Project Limitations**
### **1. ABC**
XYZ

### **2. ABC**
XYZ

### **3. ABC**
XYZ

### **4. ABC**
XYZ

<!-- Redirected link back to contents -->
### Click **[here](contents.md)** to access the project contents.