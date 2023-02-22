import cv2 #3.2.0 on BBB
import cv2.aruco as aruco
import numpy as np

# Load camera calibration data
dir = "camera_calibration/data/"
cam_matrix = np.load(dir+'cam_mtx.npy')
dist_coeff = np.load(dir+'dist.npy')

marker_length = 0.06 # marker size in m
aruco_dict_type = aruco.DICT_4X4_50
cv2.aruco_dict = aruco.Dictionary_get(aruco_dict_type)
parameters = aruco.DetectorParameters_create()

img_counter=9

camera = cv2.VideoCapture(0) # external webcam
while True:
    ret, frame = camera.read()
# frame = cv2.imread("marker_detection/marker_0.png")

    if not ret:
        print("failed to grab frame")
        break

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    corners, ids, rejected_img_points = aruco.detectMarkers(gray, cv2.aruco_dict,
                                                            parameters=parameters)
    if ids is not None and len(ids)>0:
        rvec, tvec = aruco.estimatePoseSingleMarkers(corners, marker_length, cam_matrix, dist_coeff)
        tvec = tvec.ravel()
        x = tvec[0]
        y = tvec[1]
        z = tvec[2]

        print("Marker Location [x={}, y={}, z={}]".format(x,y,z), end="\r")

        # aruco.drawDetectedMarkers(frame, corners) 

        # # Display marker coordinates on image
        # font = cv2.FONT_HERSHEY_SIMPLEX
        # cv2.putText(frame, 
        #             "x: {:.3f} y: {:.3f} z: {:.3f}".format(x, y, z), 
        #             (50, 50), 
        #             font, 0.5, 
        #             (0, 255, 255), 
        #             1, 
        #             cv2.LINE_4)

    # cv2.imshow("Marker test", frame)

    k = cv2.waitKey(1)
    if k%256 == ord('q'): # quit program
        break
    if k%256 == ord('s'): # save image
        img_name = "marker_test_{}.png".format(img_counter)
        cv2.imwrite(img_name, frame)
        print("{} written!".format(img_name))
        img_counter+=1

camera.release()
cv2.destroyAllWindows()