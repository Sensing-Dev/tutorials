import numpy as np
import datetime
from aravis import Aravis

from ionpy import Node, Builder, Buffer,  Port, Param, Type, TypeCode

if __name__ == "__main__":

    # The following info can be checked with
    # `arv-tool-0.8 -n "<name of device>" control PixelFormat Width Height PayloadSize`
    width = [1920, 1920]
    height = [1080, 1080]
    pixelformat = "Mono12"
    num_device = 2

    # the following items varies by PixelFormat
    acquisition_bb_name = "image_io_u3v_cameraN_u8x2" if pixelformat == "Mono8" \
        else "image_io_u3v_cameraN_u16x2" if pixelformat == "Mono10" or pixelformat == "Mono12" \
        else "image_io_u3v_cameraN_u8x3" if pixelformat == "RGB8" \
        else "image_io_u3v_cameraN_u8x2"
    
    bin_saver_bb_name = "image_io_binarysaver_u8x2" if pixelformat == "Mono8" \
        else "image_io_binarysaver_u16x2" if pixelformat == "Mono10" or pixelformat == "Mono12" \
        else "image_io_binarysaver_u8x3" if pixelformat == "RGB8" \
        else "image_io_binarysaver_u8x2"

    # where to save gendc
    save_data_directory = 'tutorial_save_image_bin_' + datetime.datetime.now().strftime('%Y%m%d%H%M%S')
    os.mkdir(save_data_directory)

    # pipeline setup
    builder = Builder()
    builder.set_target('host')
    builder.with_bb_module('ion-bb')

    # set port
    width_ps = []
    height_ps = []
    for i in range(num_device):
        width_ps.append(Port('width' + str(i), Type(TypeCode.Int, 32, 1), 0))
        height_ps.append(Port('height' + str(i), Type(TypeCode.Int, 32, 1), 0))

    # set params
    num_devices = Param('num_devices', num_device)
    frame_sync = Param('frame_sync', True)
    realtime_display_mode = Param('realtime_display_mode', True)
    output_directory = Param('output_directory', save_data_directory)

    # add a node to pipeline
    node = builder.add(acquisition_bb_name)\
        .set_param([num_devices, frame_sync, realtime_display_mode, ])
    
    outputs = []
    for i in range(num_device):
        chile_node = builder.add(bin_saver_bb_name)\
        .set_iport([node.get_port('output')[i], node.get_port('device_info')[i], node.get_port('frame_count')[i], width_ps[i], height_ps[i]])\
        .set_param([output_directory,
                    Param('prefix', 'image' + str(i) + '-') ])
        # create halide buffer for output port
        ith_terminator_p = chile_node.get_port('output')
        outputs.append(Buffer(Type(TypeCode.Int, 32, 1), ()))
        ith_terminator_p.bind(outputs[i])


    # bind input values to the input port
    for i in range(num_device):
        width_ps[i].bind(width[i])
        height_ps[i].bind(height[i])
    

    num_run = 0
    print("Press ctrl-C to stop saving")
    try:
        while True:
            builder.run()
            num_run += 1
    except KeyboardInterrupt:
        pass

    print( num_run, "frames are saved under", save_data_directory)
