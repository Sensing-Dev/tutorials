import numpy as np
import cv2
from aravis import Aravis

from ionpy import Node, Builder, Buffer, Port, Param, Type, TypeCode

if __name__ == "__main__":

    # device information
    width = 1920
    height = 1080
    pixelformat = "Mono8"
    num_device = 1

    # the following items varies by PixelFormat
    data_type = np.uint8 if pixelformat == "Mono8" or pixelformat == "RGB8" \
        else np.uint16 if pixelformat == "Mono10" or pixelformat == "Mono12" \
        else np.uint8
    depth_of_buffer = np.iinfo(data_type).bits
    depth_in_byte = int(depth_of_buffer / 8)
    num_bit_shift = 0 if pixelformat == "Mono8" or pixelformat == "RGB8" \
        else 4 if pixelformat == "Mono12" \
        else 6 if pixelformat == "Mono10" \
        else 0
    bb_name = "image_io_u3v_cameraN_u8x2" if pixelformat == "Mono8" \
        else "image_io_u3v_cameraN_u16x2" if pixelformat == "Mono10" or pixelformat == "Mono12" \
        else "image_io_u3v_cameraN_u8x3" if pixelformat == "RGB8" \
        else "image_io_u3v_cameraN_u8x2"
    
    # pipeline setup
    builder = Builder()
    builder.set_target('host')
    builder.with_bb_module('ion-bb')

    # set params
    num_devices = Param('num_devices', num_device)
    frame_sync = Param('frame_sync', True)
    realtime_display_mode = Param('realtime_display_mode', True)

    # add a node to pipeline
    node = builder.add(bb_name)\
        .set_params([num_devices, frame_sync, realtime_display_mode, ])
    output_p = node.get_port('output')

    # create halide buffer for output port
    output_size = (height, width, )
    if pixelformat == "RGB8":
        output_size += (3,)
    output_data = np.full(output_size, fill_value=0, dtype=data_type)
    output = Buffer(array= output_data)

    # set I/O ports
    output_p[0].bind(output)

    # prepare Opencv 
    buf_size_opencv = (height, width)
    output_byte_size = width*height*depth_in_byte
    if pixelformat == "RGB8":
        buf_size_opencv += (3,)
        output_byte_size *= 3

    coef = pow(2, num_bit_shift)
    user_input = -1

    while(user_input == -1):
        # running the builder
        builder.run()
        output_data *= coef

        cv2.imshow("img", output_data)
        user_input = cv2.waitKeyEx(1)

    cv2.destroyAllWindows()



