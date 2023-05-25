# HID Data Import
import hid # Requires "pip install hid==1.05" and "sudo apt install libhidapi-hiwraw0"

# Python thread library
import threading as th

# Regular Expression library
import regex as re

# Web Server Imports
import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

# Serial Imports
import serial as ser

# Serial-Parameters
INITIAL_TIME = 0
SERIAL_PORT_NAME = '/dev/ttyACM1'
SERIAL_BAUD_RATE = 115200

# Creating a serial port object
serial_port_data = ser.Serial(SERIAL_PORT_NAME, SERIAL_BAUD_RATE)

# Web-Server Parameters
token = "tpok3ZM1HhSkLfYjFAMa5BGSQrZSBSwtDCy8aUXNcv6KZn4Xybiuzrw8kIXaKqhL1_WL4nGY4x8dmWBK4DKZzw=="
org = "Student"
url = "https://us-east-1-1.aws.cloud2.influxdata.com"

# API Bucket Parameters
ml_bucket="ml_positional_data"

# Initialising Writing Clied
write_client = influxdb_client.InfluxDBClient(url=url, token=token, org=org)

# Define the write api
write_api = write_client.write_api(write_options=SYNCHRONOUS)

# Pedal HID values
pedal_vid = 0x063a
pedal_pid = 0x0763
pedal_path = b'/dev/hidraw1'

# Constant HID ranges
MIN_HID_VAL = 0
MAX_HID_VAL = 127
HID_MODULUS = 128

# Constant Angle Values
MIN_ANGLE_RUDDER = 0
MAX_ANGLE_RUDDER = 180

# Constant Pedal vals
RIGHT_PEDAL_OFFSET = MAX_HID_VAL
LEFT_PEDAL_OFFSET = MAX_HID_VAL
MAX_PEDAL_VAL = 127
MIN_PEDAL_VAL = 65

# Accel Constants
MAX_ACCEL = 100
MIN_ACCEL = 0
ACCEL_MODULUS = 101

# Received X & Y Values
positional_data = {
    "pos_data" : {
        "x_position" : 0,
        "y_position" : 0
    },
}

# GLOBAL STORAGES FOR PEDAL_L, PEDAL_R, PEDAL_RUDDER & x /y POS
pedal_l = 0
pedal_r = 0
pedal_rudder = 0

x = 0
y = 0

# Calculation angle of rotation (0 to 180 degrees in direction of heading
def calculate_rudder_angle(rudder_hid_val : int) -> int:

    return round((((-MAX_ANGLE_RUDDER * rudder_hid_val) / MAX_HID_VAL) + MAX_ANGLE_RUDDER))

# Calculating acceleration left/ reverse and right/forward (0% to 100%)
def calculate_left_right_accel(left_pedal_value : int, right_pedal_value : int):

    left_accel = round((MAX_ACCEL * left_pedal_value) / MAX_HID_VAL)
    right_accel = round(MAX_ACCEL - ((MAX_ACCEL * (MAX_HID_VAL - right_pedal_value))/ 63))

    # Bound the values from going below 0 or above 100
    if left_accel < 0:
        left_accel = 0
    elif left_accel > 100:
        left_accel = 100

    # Bound the values from going below 0 or above 100
    if right_accel < 0:
        right_accel = 0
    elif right_accel > 100:
        right_accel = 100

    return left_accel, right_accel

# Create a new HID Device for the pedal, given the VID, PID & Path
def create_HID_device() -> hid:
    return hid.Device(vid=pedal_vid, pid=pedal_pid, path=pedal_path)

# Display HID Debug Data
def display_debug_data(rot : int, l_acc : int, r_acc : int, abs_acc : int):
    print("Rudder Angle: {0}, Break: {1}, Accel: {2}, Abs Accel: {3}\r\n".format(rot, l_acc, r_acc, abs_acc))

# Display positional data
def display_pos_data(x : int, y : int):
    print("X pos = {0}\nY pos = {1}\n".format(x, y))

# HID Read Thread
def read_from_hid():
    global pedal_l
    global pedal_r
    global pedal_rudder

    hid_device = create_HID_device()

    while True:
        # Reading data from the pedal device
        data = hid_device.read(8)

        # Accessing from the HID data fields
        rudder_val       =   data[2] % HID_MODULUS
        left_pedal_val   =   data[0] % HID_MODULUS
        right_pedal_val  =   data[1] % HID_MODULUS


        # Setting angle and acceleration data
        left_accel, right_accel = calculate_left_right_accel(left_pedal_val, right_pedal_val)

        rudder_rotation = calculate_rudder_angle(rudder_val)

        # assigning to globals
        pedal_l = left_accel
        pedal_r = right_accel
        pedal_rudder = rudder_rotation

        # Sleep so next thread can run
        time.sleep(0.01)

# Serial write thread
def write_serial():

    # Obtaining access to global variables
    global pedal_l
    global pedal_r
    global pedal_rudder

    while True:
        
        # Flush serial port before writing
        serial_port_data.flush()

        # Write a SHELL command to serial for the NRF52840 to read & callback
        write_str = "pedal {0} {1} {2}\r\n".format(pedal_l, pedal_r, pedal_rudder)
        serial_port_data.write(write_str.encode('utf-8'))

        # Flush the buffer after we write
        serial_port_data.flush()

        # Sleep and allow other threads to run
        time.sleep(0.2)

# Serial read thread
def read_serial():

    global x
    global y
    
    while True:
        
        # Filtering the UART and colour in the serial port read
        line = serial_port_data.readline().decode("utf-8")
        line = re.sub(r'\x1b(\[.*?[@-~]|\].*?(\x07|\x1b\\))', '', line)
        line = line.replace('uart:~$ ', '')
        
        # If we experience a value conversion error
        # We want to conitnue for the next read
        try:
            # Only extract x and y from paranthesised messages read from serial
            if line[0] != '{':
                continue
            else:
                # Remove the curly braces, and split the numbers by commas
                line = line[1:-3]
                line = line.split(',')

                # Access, convert and assign the x and y values
                x_str = line[0]
                y_str = line[1]
                x = int(x_str)
                y = int(y_str)

        except ValueError:
            continue
        
        # Wrap around values that are out of bounds
        if (x >= 300) or (x <= 0):
            x = 0

        if (y >= 300) or (y <= 0):
            y = 0

        # Store the data in the dictionary
        positional_data["pos_data"]["x_position"] = x
        positional_data["pos_data"]["y_position"] = y

        # Debug Data - Not part of implementation
        print("X = {0}, Y = {1}\r\n".format(x, y))

        # Creating ML Point containing x and y values
        ml_data_point = Point("positional_data") \
            .tag("data_type", "pos_data")\
            .field("x_position", x)\
            .field("y_position", y)
        
        # # Writing to Bucket of ML x and y
        write_api.write(bucket=ml_bucket, org=org, record=ml_data_point)    

        # Sleep and allow next thread to run
        time.sleep(0.1)

    

# Initialise threads with function references
hid_thread = th.Thread(target=read_from_hid)
serial_write_thread = th.Thread(target=write_serial) 
serial_read_thread = th.Thread(target=read_serial)

# Start threads
hid_thread.start()
serial_write_thread.start()
serial_read_thread.start()