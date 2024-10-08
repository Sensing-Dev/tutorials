import numpy as np
import datetime
import os
from ionpy import Node, Builder, Buffer, Port, Param, Type, TypeCode

if __name__ == "__main__":

    # The following info can be checked with
    # `arv-tool-0.8 -n "<name of device>" control PixelFormat Width Height PayloadSize`
    width = 640
    height = 480
    pixelformat = "Mono8"
    num_device = 2

    # the following items varies by PixelFormat
    acquisition_bb_name = "image_io_u3v_cameraN_u8x2" if pixelformat == "Mono8" \
        else "image_io_u3v_cameraN_u16x2" if pixelformat == "Mono16" \
        else "image_io_u3v_cameraN_u8x2"

    bin_saver_bb_name = "image_io_binarysaver_u8x2" if pixelformat == "Mono8" \
        else "image_io_binarysaver_u16x2" if pixelformat == "Mono16" \
        else "image_io_binarysaver_u8x2"

    # where to save gendc
    save_data_directory = 'tutorial_save_image_bin_' + datetime.datetime.now().strftime('%Y%m%d%H%M%S')
    os.mkdir(save_data_directory)

    # pipeline setup
    builder = Builder()
    builder.set_target('host')
    builder.with_bb_module('ion-bb')

    # set port
    width_p = Port('width', Type(TypeCode.Int, 32, 1), 0)
    height_p = Port('height', Type(TypeCode.Int, 32, 1), 0)

    # set params
    num_devices = Param('num_devices', num_device)
    output_directory = Param('output_directory', save_data_directory)

    # add a node to pipeline

    node = builder.add(acquisition_bb_name) \
        .set_params([num_devices, Param('width', width), Param('height', height)])

    outputs = []
    for i in range(num_device):
        chile_node = builder.add(bin_saver_bb_name) \
            .set_iports(
            [node.get_port('output')[i], node.get_port('device_info')[i], node.get_port('frame_count')[i], width_p,
             height_p]) \
            .set_params([output_directory,
                         Param('prefix', 'image' + str(i) + '-')])
        # create halide buffer for output port
        ith_terminator_p = chile_node.get_port('output')
        outputs.append(Buffer(Type(TypeCode.Int, 32, 1), ()))
        ith_terminator_p.bind(outputs[i])

    # bind input values to the input port
    width_p.bind(width)
    height_p.bind(height)

    num_run = 0

    for i in range(100):
        builder.run()
        num_run += 1

    print(num_run, "frames are saved under", save_data_directory)
