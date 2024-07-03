import numpy as np
import cv2

import os
os.add_dll_directory(os.path.join(os.environ["SENSING_DEV_ROOT"], "bin"))

import gi
gi.require_version("Aravis", "0.8")
from gi.repository import Aravis

from ionpy import Node, Builder, Buffer, PortMap, Port, Param, Type, TypeCode

if os.name == 'nt':
    module_name = 'ion-bb.dll'
elif os.name == 'posix':
    module_name = 'libion-bb.so'

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
    builder.with_bb_module(module_name)

    # set port
    dispose_p = Port('dispose', Type(TypeCode.Uint, 1, 1), 0)
    gain_p = Port('gain', Type(TypeCode.Float, 64, 1), 1)
    exposuretime_p = Port('exposuretime', Type(TypeCode.Float, 64, 1), 1)

    # set params
    num_devices = Param('num_devices', str(num_device))
    pixel_format_ptr = Param('pixel_format_ptr', pixelformat)
    frame_sync = Param('frame_sync', 'true')
    gain_key = Param('gain_key', 'Gain')
    exposure_key = Param('exposure_key', 'ExposureTime')
    realtime_diaplay_mode = Param('realtime_diaplay_mode', 'true')

    # add a node to pipeline
    node = builder.add(bb_name)\
        .set_port([dispose_p, gain_p, exposuretime_p, ])\
        .set_param([num_devices, pixel_format_ptr, frame_sync, gain_key, exposure_key, realtime_diaplay_mode, ])
    output_p = node.get_port('output')

    # create halide buffer for input port
    gain_data = np.array([48.0])
    exposure_data = np.array([100.0])

    gains = Buffer(Type(TypeCode.Float, 64, 1), (1,))
    exposures = Buffer(Type(TypeCode.Float, 64, 1), (1,))
    gains.write(gain_data.tobytes(order='C'))
    exposures.write(exposure_data.tobytes(order='C'))

    # create halide buffer for output port
    outputs = []
    output_size = (width, height, )
    if pixelformat == "RGB8":
        output_size += (3,)
    outputs.append(Buffer(Type(TypeCode.Uint, depth_of_buffer, 1), output_size))

    # set I/O ports
    port_map = PortMap()
    port_map.set_buffer(gain_p, gains)
    port_map.set_buffer(exposuretime_p, exposures)
    port_map.set_buffer_array(output_p, outputs)

    # prepare Opencv 
    buf_size_opencv = (height, width)
    output_byte_size = width*height*depth_in_byte
    if pixelformat == "RGB8":
        buf_size_opencv += (3,)
        output_byte_size *= 3

    loop_num = 100
    user_input = -1
    while(True):
        port_map.set_u1(dispose_p, user_input!=-1)
        # running the builder
        builder.run(port_map)
        
        if user_input!=-1:
            break
        
        output_bytes = outputs[0].read(output_byte_size) 

        output_np_HxW = np.frombuffer(output_bytes, data_type).reshape(buf_size_opencv)
        output_np_HxW *= pow(2, num_bit_shift)

        cv2.imshow("A", output_np_HxW)
        user_input = cv2.waitKeyEx(1)  # enter eny key to exit


    cv2.destroyAllWindows()



