import os

if os.name == 'nt':  # windows
    os.add_dll_directory(os.path.join(os.environ["SENSING_DEV_ROOT"], "bin"))
# linux: please install sensing-dev c++ version and export GST_PLUGIN_PATH=/opt/sensing-dev/lib/x86_64-linux-gnu/gstreamer-1.0
import cv2

if __name__ == "__main__":

    camera_name = ''
    # camera_name = 'camera-name'
    pixelformat = 'Mono8'
    # pixelformat = 'BayerBG8'
    width = 1920
    height = 1080
    framerate = 60
    is_color = False
    required_bits = 8 if pixelformat == "Mono8" or pixelformat == "RGB8" \
        else 16 if pixelformat == "Mono10" or pixelformat == "Mono12" \
        else 8

    num_bit_shift = 0 if pixelformat == "Mono8" or pixelformat == "RGB8" \
        else 4 if pixelformat == "Mono12" \
        else 6 if pixelformat == "Mono10" \
        else 0
    
    if pixelformat.startswith("RGB") or pixelformat.startswith("Bayer") :
        is_color = True

    if is_color:
        pipeline = 'aravissrc camera-name="{}" ! \
        gendcseparator name=sep ! queue ! fakesink \
        sep.component_src0 ! queue ! video/x-bayer,format=bggr,width={},height={},framerate={}/1 ! bayer2rgb ! videoconvert ! appsink'.format(
            camera_name, width, height, framerate)
    else:
        if required_bits == 8:
            pipeline = 'aravissrc camera-name="{}" ! \
            gendcseparator name=sep ! queue ! fakesink \
            sep.component_src0 ! queue ! video/x-raw,format=GRAY8,width={},height={},framerate={}/1 ! videoconvert ! appsink'.format(
                camera_name, width, height, framerate)
        else:
            pipeline = 'aravissrc camera-name="{}" ! \
                       gendcseparator name=sep ! queue ! fakesink \
                       sep.component_src0 ! queue ! video/x-raw,format=GRAY16_LE,depth={},width={},height={},framerate={}/1 ! videoconvert ! appsink'.format(
                camera_name, required_bits-num_bit_shift, width, height, framerate)


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
