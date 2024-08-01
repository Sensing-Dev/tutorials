import numpy as np
import datetime
from aravis import Aravis

from ionpy import Node, Builder, Buffer,  Port, Param, Type, TypeCode
import os

if __name__ == "__main__":

    # The following info can be checked with
    # `arv-tool-0.8 -n "<name of device>" control PixelFormat Width Height PayloadSize`
    width = 1920
    height = 1080
    pixelformat = "Mono8"
    payloadsize = [2074880, 2074880]

    num_device = 2

    # where to save gendc
    save_data_directory = 'tutorial_save_gendc_' + datetime.datetime.now().strftime('%Y%m%d%H%M%S')
    os.mkdir(save_data_directory)

    # pipeline setup
    builder = Builder()
    builder.set_target('host')
    builder.with_bb_module('ion-bb')

    # set port
    payloadsize_ps = []
    for i in range(num_device):
        payloadsize_ps.append(Port('payloadsize' + str(i), Type(TypeCode.Int, 32, 1), 0))

    # set params
    num_devices = Param('num_devices', num_device)
    frame_sync = Param('frame_sync', False)
    realtime_display_mode = Param('realtime_diaplay_mode', True)
    output_directory = Param('output_directory', save_data_directory)

    # add a node to pipeline
    node = builder.add("image_io_u3v_gendc")\
        .set_params([num_devices, frame_sync, realtime_display_mode, ])

    t_node0 = builder.add("image_io_binary_gendc_saver")\
        .set_iports([node.get_port('gendc')[0], node.get_port('device_info')[0], payloadsize_ps[0], ])\
        .set_params([output_directory,
                    Param('prefix', 'gendc0-') ])

    # create halide buffer for output port
    terminator0 = t_node0.get_port('output')
    output0 = Buffer(Type(TypeCode.Int, 32, 1), ())
    terminator0.bind(output0)


    if num_device ==2 :
        t_node1 = builder.add("image_io_binary_gendc_saver") \
            .set_iports([node.get_port('gendc')[1], node.get_port('device_info')[1], payloadsize_ps[1], ]) \
            .set_params([output_directory,
                        Param('prefix', 'gendc1-')])
        # create halide buffer for output port
        terminator1 = t_node1.get_port('output')
        output1 = Buffer(Type(TypeCode.Int, 32, 1), ())
        terminator1.bind(output1)

    # bind input values to the input port
    for i in range(num_device):
        payloadsize_ps[i].bind(payloadsize[i])
    

    num_run = 0
    print("Press ctrl-C to stop saving")
    try:
        while True:
            builder.run()
            num_run += 1
    except KeyboardInterrupt:
        pass

    print( num_run, "frames are saved under", save_data_directory)
