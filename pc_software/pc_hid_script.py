import hid
import subprocess
import threading as th

# Web Server Imports
import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

# Serial Imports
import serial as ser

# If this does not compile, run this:
installed = True

if not installed:
    # Install hid module using pip
    pip_install = subprocess.run(['pip', 'install', 'hid==1.05'], capture_output=True, text=True)
    print(pip_install.stdout)

    # Install libhidapi-hidraw0 package using apt
    apt_install = subprocess.run(['sudo', 'apt', 'install', 'libhidapi-hidraw0'], capture_output=True, text=True)
    print(apt_install.stdout)

# Serial-Parameters
INITIAL_TIME = 0
SERIAL_PORT_NAME = '/dev/ttyACM3'
SERIAL_BAUD_RATE = 115200

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

# GLOBAL STORAGES FOR PEDAL_L, PEDAL_R, PEDAL_RUDDER
pedal_l = 0
pedal_r = 0
pedal_rudder = 0

# Calculation angle of rotation (0 to 180 degrees in direction of heading
def calculate_rudder_angle(rudder_hid_val : int) -> int:

    return round((((-MAX_ANGLE_RUDDER * rudder_hid_val) / MAX_HID_VAL) + MAX_ANGLE_RUDDER))

# Calculating acceleration left/ reverse and right/forward (0% to 100%)
def calculate_left_right_accel(left_pedal_value : int, right_pedal_value : int):

    left_accel = round((MAX_ACCEL * left_pedal_value) / MAX_HID_VAL) % ACCEL_MODULUS
    right_accel = round(MAX_ACCEL - ((MAX_ACCEL * (MAX_HID_VAL - right_pedal_value))/ 63)) % ACCEL_MODULUS

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

# ----- ASYNC FUNCS -----

# Asynchronous reading from HID
# IMPLEMENT TOMORROW
def read_from_hid():

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

        absolute_accel =  right_accel - left_accel

        # Displaying debug data
        # display_debug_data(rudder_rotation, left_accel, right_accel, absolute_accel)

        # assigning to globals
        pedal_l = left_accel
        pedal_r = right_accel
        pedal_rudder = rudder_rotation

# Asynchronous Reading & Writing From Serial
def write_serial():

    while True:
        current_line = serial_port_data.readline().decode("utf-8").strip()
        print(current_line)

        # time.sleep(0.01)
        write_str = "pedal {0} {1} {2}".format(pedal_l, pedal_r, pedal_rudder)
        print(write_str)
        serial_port_data.write(write_str.encode('utf-8'))

        for i in range(2):
            for j in range(len(write_str)):
                serial_port_data.write(write_str[j].encode("utf-8"))
            serial_port_data.write('\n'.encode("utf-8"))

        


        serial_port_data.flush()

def read_serial():
    while True:
        a = serial_port_data.read(1).decode("utf-8")
        if a == '{':
            xval = []
            a = serial_port_data.read(1).decode("utf-8")
            while a != ',':
                xval.append(a)
                a = serial_port_data.read(1).decode("utf-8")
            x = 0
            for i in range(len(xval)):
                x += int(xval[i]) * (10 ** (len(xval) - i - 1))

            yval = []
            a = serial_port_data.read(1).decode("utf-8")
            while a != '}':
                yval.append(a)
                a = serial_port_data.read(1).decode("utf-8")
            y = 0
            for i in range(len(yval)):
                y += int(yval[i]) * (10 ** (len(yval) - i - 1))

        positional_data["pos_data"]["x_position"] = x
        positional_data["pos_data"]["y_position"] = y

        # # Creating ML Point containing x and y values
        # ml_data_point = Point("positional_data") \
        #     .tag("data_type", "pos_data")\
        #     .field("x_position", positional_data["pos_data"]["x_position"])\
        #     .field("y_position", positional_data["pos_data"]["y_position"])
        
        # # # Writing to Bucket of ML x and y
        # write_api.write(bucket=ml_bucket, org=org, record=ml_data_point)     

hid_thread = th.Thread(target=read_from_hid)
serial_write_thread = th.Thread(target=write_serial) 
serial_read_thread = th.Thread(target=read_serial)

hid_thread.start()
serial_write_thread.start()
serial_read_thread.start()