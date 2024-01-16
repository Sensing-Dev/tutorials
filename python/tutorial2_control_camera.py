import numpy as np
import cv2

import os
os.add_dll_directory(os.path.join(os.environ["SENSING_DEV_ROOT"], "bin"))

import gi
gi.require_version("Aravis", "0.8")
from gi.repository import Aravis

from ionpy import Node, Builder, Buffer, PortMap, Port, Param, Type, TypeCode

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

    gain_ps = []
    exposure_ps = []
    for i in range(num_device):
        gain_ps.append(Port('gain' + str(i), Type(TypeCode.Float, 64, 1), 0))
        exposure_ps.append(Port('exposure' + str(i), Type(TypeCode.Float, 64, 1), 0))

    # set params
    num_devices = Param('num_devices', str(num_device))
    frame_sync = Param('frame_sync', 'false')
    realtime_diaplay_mode = Param('realtime_diaplay_mode', 'true')
    enable_control = Param('enable_control', 'true')
    gain_key = Param('gain_key', 'Gain')
    exposure_key = Param('exposure_key', 'ExposureTime')

    gain_values = []
    exposure_values = []

    for i in range(num_device):
        gain_values.append(40.0)
        exposure_values.append(100.0)

    # add a node to pipeline
    node = builder.add(bb_name)\
        .set_iport([gain_ps[0], exposure_ps[0]])\
        .set_param([num_devices, frame_sync, realtime_diaplay_mode, enable_control, gain_key, exposure_key]) if num_device == 1 \
        else builder.add(bb_name)\
            .set_iport([gain_ps[0], exposure_ps[0], gain_ps[1], exposure_ps[1]])\
            .set_param([num_devices, frame_sync, realtime_diaplay_mode, enable_control, gain_key, exposure_key])
    output_p = node.get_port('output')

    # create halide buffer for output port
    outputs = []
    output_size = (width, height, )
    if pixelformat == "RGB8":
        output_size += (3,)
    for i in range(num_device):
        outputs.append(Buffer(Type(TypeCode.Uint, depth_of_buffer, 1), output_size))

    # set I/O ports
    for i in range(num_device):
        gain_ps[i].bind(gain_values[i])
        exposure_ps[i].bind(exposure_values[i])
        output_p[i].bind(outputs[i])

    # prepare Opencv 
    buf_size_opencv = (height, width)
    output_byte_size = width*height*depth_in_byte
    if pixelformat == "RGB8":
        buf_size_opencv += (3,)
        output_byte_size *= 3

    loop_num = 100
    coef = pow(2, num_bit_shift)

    for x in range(loop_num):
        # running the builder
        builder.run()
        for i in range(num_device):
            output_bytes = outputs[i].read(output_byte_size) 

            output_np_HxW = np.frombuffer(output_bytes, data_type).reshape(buf_size_opencv)
            output_np_HxW *= coef

            cv2.imshow("img" + str(i), output_np_HxW)
        cv2.waitKey(1)

    cv2.destroyAllWindows()



