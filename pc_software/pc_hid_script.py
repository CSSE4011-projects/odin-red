import hid
import subprocess

# If this does not compile, run this:
installed = True

if not installed:
    # Install hid module using pip
    pip_install = subprocess.run(['pip', 'install', 'hid==1.05'], capture_output=True, text=True)
    print(pip_install.stdout)

    # Install libhidapi-hidraw0 package using apt
    apt_install = subprocess.run(['sudo', 'apt', 'install', 'libhidapi-hidraw0'], capture_output=True, text=True)
    print(apt_install.stdout)

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

def display_debug_data(rot, l_acc, r_acc, abs_acc):
    print("Rudder Angle: {0}, Break: {1}, Accel: {2}, Abs Accel: {3}\r\n".format(rot, l_acc, r_acc, abs_acc))

# Read data from the device
while 1:

    hid_device = create_HID_device()

    # Reading data from the pedal device
    data = hid_device.read(8)

    # Accessing from the HID data fields
    rudder_val       =   data[2] % HID_MODULUS
    left_pedal_val   =   data[0] % HID_MODULUS
    right_pedal_val  =   data[1] % HID_MODULUS


    # Setting angle and acceleration data
    accel_array = calculate_left_right_accel(left_pedal_val, right_pedal_val)

    left_accel, right_accel = calculate_left_right_accel(left_pedal_val, right_pedal_val)

    rudder_rotation = calculate_rudder_angle(rudder_val)

    absolute_accel =  right_accel - left_accel

    # Displaying debug data
    display_debug_data(rudder_rotation, left_accel, right_accel, absolute_accel)

      
