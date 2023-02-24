import cv2 #3.2.0 on BBB
import cv2.aruco as aruco
import numpy as np
from math

import GantryControl

CAM_X_OFFSET_MM = 50.0
CAM_Y_OFFSET_MM = 50.0
Z_MOUNTING_OFFSET_MM = 38.2 # + camera to gantry in z direction
ARM_LENGTH_MM = 346.3
MAX_Z = Z_MOUNTING_OFFSET_MM + ARM_LENGTH_MM

# Aruco Marker Information
MARKER_LENGTH = 0.06 # marker size in m
aruco_dict_type = aruco.DICT_4X4_50
cv2.aruco_dict = aruco.Dictionary_get(aruco_dict_type)
parameters = aruco.DetectorParameters_create()

# Load camera calibration data
dir = "camera_calibration/data/"
cam_matrix = np.load(dir+'cam_mtx.npy')
dist_coeff = np.load(dir+'dist.npy')

camera = cv2.VideoCapture(0)

def capture_frame(camera):
    ret, frame = camera.read()

    if not ret:
        return None

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    return gray

def calculate_xz_cmd(z_dist_mm):
    z_arm = z_dist_mm-Z_MOUNTING_OFFSET_MM
    theta = math.degrees(math.asin(z_arm/ARM_LENGTH_MM))
    x = math.sqrt(ARM_LENGTH_MM*ARM_LENGTH_MM - z_dist_mm*z_dist_mm)

    return theta, x

def calculate_command(image):
    corners, ids, rejected_img_points = aruco.detectMarkers(image, cv2.aruco_dict,
                                                            parameters=parameters)

    if ids is not None and len(ids)>0:
        rvec, tvec = aruco.estimatePoseSingleMarkers(corners, MARKER_LENGTH, cam_matrix, dist_coeff)
        tvec = tvec.ravel()
        # Positions to top left corner of marker
        x_mm = tvec[0]*1000 # x pos dir: right
        y_mm = tvec[1]*1000 # y pos dir: down
        z_mm = tvec[2]*1000

    y_out_mm = CAM_Y_OFFSET_MM - y_mm

    # Check if bike is reachable
    if z_mm > MAX_Z:
        return None, None, None

    z_out_deg, x_arm = calculate_xz_cmd(z_mm)
    x_out_mm = CAM_X_OFFSET_MM - x_mm - x_arm

    if x_out_mm < 0:
        return None, None, None
    
    return x_out_mm, y_out_mm, z_out_deg

    
