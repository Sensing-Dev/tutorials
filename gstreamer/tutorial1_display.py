import os
if os.name == 'nt':
    os.add_dll_directory(os.path.join(os.environ["SENSING_DEV_ROOT"], "bin"))
# import cv2

if __name__ == "__main__":
    camera_name = ''
    pixelformat = 'GRAY8'
    width = 1920
    height = 1080
    framerate = 60

    pipeline = 'aravissrc camera-name={} ! \
    gendcseparator name=sep ! queue ! fakesink \
    sep.component_src0 ! queue ! video/x-raw,format={},width={},height={},framerate={}/1 ! videoconvert ! appsink'.format(camera_name, pixelformat, width, height, framerate)

    print(pipeline)

    cap = cv2.VideoCapture(pipeline, cv2.CAP_GSTREAMER)

    user_input = -1
    print('press any key on preview window to stop')

    while (user_input == -1):
        ret, frame = cap.read()
        if not ret:
            print("Can't receive frame")
            break

        cv2.imshow('opencv with gstreamer test', frame)
        user_input = cv2.waitKeyEx(1)
    cv2.destroyAllWindows()
    cap.release()