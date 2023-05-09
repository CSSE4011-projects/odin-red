# Rover Deployment & Control

## **Rover Positioning**
The rover will be deployed with a 2 metre by 2 metre grid, and this is the only configured region of operation for the rover. This rover will be pedal controlled, and these controls will be sent to the rover using BLE communication. The rover will then respond to these instructions and move accordingly. 

## **Rover Tracking**
The rover will be tracked utilising a number of sensors, namely, the Time of Flight (ToF) and magentometer sensors. These sensors provide us with distance to the closest barrier, and the angle of rotation, allowing the relative x and y position to be calculated through trigonometry.

Since we have four (4) Time of Flight sensors, we will have 4 distances to the walls, for all faces of the rover, these will be $d_1, \space d_2, \space d_3 \space and \space d_1 $, where each distance is at an anlgle $\theta$ from the horizontal, therefore, the calculations of the corresponding x and y components are given as:

1.  $\space y_1 = d_1 sin(\theta), \space y_2 = d_2 sin(\theta)$

2. $\space x_1 = d_3 sin(\theta), \space  x_2 = d_4 sin(\theta)$ </br>

These values (x and y positions) will be filtered utilising the Kalman Filter, to provide more accurate and reliable positioning data.


## **Rover Pedal Control**
### **Left Pedal Control**
- Forward controls the forward movement of the two left wheels of the rover
- Backward controls the backward movement of the two left wheels of the rover.
### **Right Pedal Control**
- Forward controls the forward movement of the two right wheels of the rover
- Backward controls the backward movement of the two right wheels of the rover.



<p align="center">
  <img src=rover_grid.png alt="Rover example positioning in grid" height=500/>
</p>

<!-- Redirected link back to contents -->
### Click **[here](contents.md)** to access the project contents.