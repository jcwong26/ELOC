import cv2 #3.2.0 on BBB
import cv2.aruco as aruco
import numpy as np
import math
import time
import serial
from enum import Enum

from gantry_control import GantryControl

# TODO: check port names
GANTRY_PORT = '/dev/ttyUSB0'
LV_PORT = '/dev/ttyACM1'
WPT_PORT = '/dev/ttyACM0'

CAM_X_OFFSET_MM = 420
CAM_Z_OFFSET_MM = 215
Y_MOUNTING_OFFSET_MM = 287 # + camera to gantry in z direction 16.7+2.5+9.5
ARM_LENGTH_MM = 260
MAX_Y = Y_MOUNTING_OFFSET_MM + ARM_LENGTH_MM
THETA_OFFSET = 0

# Aruco Marker Information
MARKER_LENGTH = 0.06 # marker size in m
aruco_dict_type = aruco.DICT_4X4_50
cv2.aruco_dict = aruco.Dictionary_get(aruco_dict_type)
parameters = aruco.DetectorParameters_create()
 
# Load camera calibration data
dir = "/home/debian/ELOC/camera_calibration/data/"
cam_matrix = np.load(dir+'cam_mtx.npy')
dist_coeff = np.load(dir+'dist.npy')

LV_STATES = ["UNLOCKEDEM_state",
            "LOADING_state",
            "CLOSED_state",
            "CHARGING_state",
            "UNLOCKED_state",
            "UNLOADING_state",
            "EMPTY_state"]

def capture_frame(camera):
    ret, frame = camera.read()

    if not ret:
        return None

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    return gray

def calculate_xy_cmd(y_dist_mm):
    y_arm = y_dist_mm-Y_MOUNTING_OFFSET_MM
    if y_arm > ARM_LENGTH_MM: # target too far to reach, most likely bad reading
        return -1, -1
    else:
        theta = math.degrees(math.asin(y_arm/ARM_LENGTH_MM)) - THETA_OFFSET
        x = math.sqrt(ARM_LENGTH_MM*ARM_LENGTH_MM - y_arm*y_arm)

    return theta, x

def calculate_command(image):
    corners, ids, rejected_img_points = aruco.detectMarkers(image, cv2.aruco_dict,
                                                            parameters=parameters)

    if ids is not None and ids[0]==39 and len(ids)==1:
        rvec, tvec = aruco.estimatePoseSingleMarkers(corners, MARKER_LENGTH, cam_matrix, dist_coeff)
        tvec = tvec.ravel()
        # Positions to top left corner of marker
        x_cam_mm = tvec[0]*1000 # camera x pos dir: right
        z_cam_mm = -tvec[1]*1000 # camera y pos dir: down
        y_cam_mm = tvec[2]*1000
        print("Marker Location [x_cam_mm={}, z_cam_mm={}, y_cam_mm={}]".format(x_cam_mm, z_cam_mm, y_cam_mm))
    else:
        return -1, -1, -1

    # Check if bike is reachable
    # if y_cam_mm > MAX_Y:
    #     return None, None, None

    z_out_mm = CAM_Z_OFFSET_MM + z_cam_mm # vertical direction

    y_out_deg, x_arm = calculate_xy_cmd(y_cam_mm)
    x_out_mm = CAM_X_OFFSET_MM - x_cam_mm - x_arm

    # if x_out_mm < 0:
    #     return None, None, None

    return x_out_mm, z_out_mm, y_out_deg

def move_to_charger(gantry, camera):
    x_cmd_list = []
    z_cmd_list = []
    y_cmd_list = []
    for i in range(30):
        img = capture_frame(camera)
        x, z, y = calculate_command(img)
        if x > 0:
            x_cmd_list.append(x)
        if z > 0:
            z_cmd_list.append(z)
        if y > 0:
            y_cmd_list.append(y)

    x_cmd_mm = np.average(x_cmd_list)
    z_cmd_mm = np.average(z_cmd_list)
    y_cmd_deg = np.average(y_cmd_list)

    print("Gantry Command (avg)")
    print("x_cmd_mm = {}".format(x_cmd_mm))
    print("z_cmd_mm = {}".format(z_cmd_mm))
    print("y_cmd_deg = {}".format(y_cmd_deg))

    gantry.auto_home_xz()
    gantry.move_xz(x_cmd_mm, z_cmd_mm)
    gantry.move_arm(y_cmd_deg)

    return x_cmd_mm

def move_gantry_back(gantry, x_cmd_mm):
    gantry.move_x(x_cmd_mm-80)
    gantry.auto_home_arm()
    gantry.move_xz(0, 0)

def write_to_lv(transition):
    lv.write(str.encode("{}\r".format(transition)))
    
def write_to_wpt(command):
    wpt.write(str.encode("{}\r".format(command)))

# Initialize camera and gantry
print("Initializing camera and connecting to boards")
camera = cv2.VideoCapture(0)
gantry = GantryControl(GANTRY_PORT)

lv = serial.Serial(port=LV_PORT, baudrate=115200)
lv.close()
lv.open()

wpt = serial.Serial(port=WPT_PORT, baudrate=115200)
wpt.close()
wpt.open()

time.sleep(2)
gantry.auto_home_arm()
gantry.auto_home_xz()

# GANTRY TEST CODE
# move_to_charger(gantry, camera)
# write_to_wpt("startCharging")
# write_to_lv("rainbow_chase_start")
# time.sleep(30)
# write_to_lv("rainbow_chase_stop")
# write_to_wpt("stopCharging")
# move_gantry_back(gantry)


current_state = "EMPTY_state"
inputs = []
last_x = 0
print("START")
while (True):
    # Check if incoming bytes are waiting to be read from serial input buffer
    if (lv.inWaiting() != 0):
        inputs = lv.read(lv.inWaiting()).decode('utf-8').split('\r\n')
        for input in inputs:
            print(input)
            if input in LV_STATES:
                current_state = input
                print("BB CHANGED STATES")
            
            elif input == "COMPVISION_state" and current_state == "CLOSED_state":
                print("BB COMPVISION_state")
                time.sleep(5)
                last_x = move_to_charger(gantry, camera)
                write_to_wpt("startCharging")
                write_to_lv("to_charging")
                current_state = input

            elif input == "WaitForBBBFin_state" and current_state == "CHARGING_state":
                print("BB WaitForBBBFin_state")
                write_to_wpt("stopCharging")
                move_gantry_back(gantry, last_x)
                write_to_lv("to_unlocked")
                current_state = input
        