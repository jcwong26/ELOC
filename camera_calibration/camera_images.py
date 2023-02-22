# Capture images from webcam to use for calibration

import cv2

camera = cv2.VideoCapture(1) # external webcam
img_counter = 0
folder_name = "webcam_calibration_images"
while True:
    ret, frame = camera.read()
    if not ret:
        print("failed to grab frame")
        break
    cv2.imshow("test", frame)

    k = cv2.waitKey(1)
    if k%256 == 27:
        # ESC pressed
        print("Escape hit, closing...")
        break
    elif k%256 == 32:
        # SPACE pressed
        img_name = "marker_{}.png".format(img_counter)
        cv2.imwrite(img_name, frame)
        print("{} written!".format(img_name))
        img_counter += 1

camera.release()
cv2.destroyAllWindows()