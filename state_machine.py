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
LV_PORT = '/dev/tty/ACM1'
WPT_PORT = '/dev/tty/ACM0'

CAM_X_OFFSET_MM = 50.0
CAM_Z_OFFSET_MM = 50.0
Y_MOUNTING_OFFSET_MM = 260 # + camera to gantry in z direction
ARM_LENGTH_MM = 260
MAX_Y = Y_MOUNTING_OFFSET_MM + ARM_LENGTH_MM

# Aruco Marker Information
MARKER_LENGTH = 0.06 # marker size in m
aruco_dict_type = aruco.DICT_4X4_50
cv2.aruco_dict = aruco.Dictionary_get(aruco_dict_type)
parameters = aruco.DetectorParameters_create()
 
# Load camera calibration data
dir = "camera_calibration/data/"
cam_matrix = np.load(dir+'cam_mtx.npy')
dist_coeff = np.load(dir+'dist.npy')

LV_STATES = ["LOADING_state",
            "CLOSED_state",
            "COMPVISION_state",
            "CHARGING_state",
            "UNLOCKED_state",
            "UNLOADING_state",
            "EMPTY_state"
]

def capture_frame(camera):
    ret, frame = camera.read()

    if not ret:
        return None

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    return gray

def calculate_xy_cmd(y_dist_mm):
    y_arm = y_dist_mm-Y_MOUNTING_OFFSET_MM
    theta = math.degrees(math.asin(y_arm/ARM_LENGTH_MM))
    x = math.sqrt(ARM_LENGTH_MM*ARM_LENGTH_MM - y_dist_mm*y_dist_mm)

    return theta, x

def calculate_command(image):
    corners, ids, rejected_img_points = aruco.detectMarkers(image, cv2.aruco_dict,
                                                            parameters=parameters)

    if ids is not None and len(ids)>0:
        rvec, tvec = aruco.estimatePoseSingleMarkers(corners, MARKER_LENGTH, cam_matrix, dist_coeff)
        tvec = tvec.ravel()
        # Positions to top left corner of marker
        x_cam_mm = tvec[0]*1000 # camera x pos dir: right
        z_cam_mm = -tvec[1]*1000 # camera y pos dir: down
        y_cam_mm = tvec[2]*1000
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
    x_cmd_mm = z_cmd_mm = y_cmd_deg = -1
    while x_cmd_mm == -1:
        img = capture_frame(camera)
        x_cmd_mm, z_cmd_mm, y_cmd_deg = calculate_command(img)

    gantry.move_xz(x_cmd_mm, z_cmd_mm)
    gantry.move_arm(y_cmd_deg)

def move_gantry_back(gantry):
    gantry.auto_home_arm()
    gantry.move_xz(0, 0)

def write_to_lv(transition):
    lv.write("{}\r".format(transition))
    
def write_to_wpt(command):
    wpt.write("{}\r".format(command))

# Initialize camera and gantry
camera = cv2.VideoCapture(0)
gantry = GantryControl(GANTRY_PORT)
time.sleep(2)
gantry.auto_home_arm()
gantry.auto_home_xz()

lv = serial.Serial(port=LV_PORT, baudrate=115200)
wpt = serial.Serial(port=WPT_PORT, baudrate=115200)

current_state = "EMPTY_state"
input = []

while (True):
    # Check if incoming bytes are waiting to be read from serial input buffer
    if (lv.in_waiting() > 0):
        inputs = lv.read(lv.in_waiting()).decode('utf-8').split('\r\n')

        for input in inputs:
            if input in LV_STATES:
                current_state = input
            
            elif input == "COMPVISION_state" & current_state == "CLOSED_state":
                move_to_charger(gantry, camera)
                write_to_wpt("startCharging")
                write_to_lv("to_charging")
                current_state = input

            elif input == "WaitForBBBFin_state" & current_state == "CHARGING_state":
                write_to_wpt("stopCharging")
                move_gantry_back(gantry)
                write_to_lv("to_unlocked")
                current_state = input

        