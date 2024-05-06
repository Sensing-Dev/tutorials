import numpy as np
import datetime

import os
if os.name == 'nt':
    os.add_dll_directory(os.path.join(os.environ["SENSING_DEV_ROOT"], "bin"))

from ionpy import Node, Builder, Buffer, PortMap, Port, Param, Type, TypeCode

if __name__ == "__main__":

    # The following info can be checked with
    # `arv-tool-0.8 -n "<name of device>" control PixelFormat Width Height PayloadSize`
    width = 1920
    height = 1080
    pixelformat = "Mono8"
    payloadsize = 2074880

    num_device = 1

    # where to save gendc
    save_data_directory = 'tutorial_save_gendc_' + datetime.datetime.now().strftime('%Y%m%d%H%M%S')
    os.mkdir(save_data_directory)

    # pipeline setup
    builder = Builder()
    builder.set_target('host')
    builder.with_bb_module('ion-bb')

    # set port
    payloadsize_p = Port('payloadsize', Type(TypeCode.Int, 32, 1), 0)

    # set params
    num_devices = Param('num_devices', str(num_device))
    frame_sync = Param('frame_sync', 'false')
    realtime_diaplay_mode = Param('realtime_diaplay_mode', 'true')
    output_directory = Param('output_directory', save_data_directory)

    # add a node to pipeline
    node = builder.add("image_io_u3v_gendc")\
        .set_param([num_devices, frame_sync, realtime_diaplay_mode, ])

    node_sensor0 = builder.add("image_io_binary_gendc_saver")\
        .set_iport([node.get_port('gendc')[0], node.get_port('device_info')[0], payloadsize_p, ])\
        .set_param([output_directory, 
                    Param('prefix', 'sensor0-') ])

    # create halide buffer for output port
    terminator0 = node_sensor0.get_port('output')
    output0 = Buffer(Type(TypeCode.Int, 32, 1), ())
    terminator0.bind(output0)

    # bind input values to the input port
    payloadsize_p.bind(payloadsize)

    # if num_devices == 2:
    #     payloadsize1_p = Port('payloadsize1', Type(TypeCode.Int, 32, 1), 0)
    #     node_sensor1 = builder.add("image_io_binary_gendc_saver")\
    #         .set_iport([node.get_port('gendc')[1], node.get_port('device_info')[1], payloadsize1_p, ])\
    #         .set_param([output_directory, 
    #                     Param('prefix', 'sensor1-') ])
    #     terminator1 = node_sensor1.get_port('output')
    #     output1 = Buffer(Type(TypeCode.Int, 32, 1), ())
    #     terminator1.bind(output1)
    #     payloadsize1_p.bind(payloadsize)

    num_run = 0
    print("Press any key to stop saving")
    try:
        while True:
            builder.run()
            num_run += 1
    except KeyboardInterrupt:
        pass

    print( num_run, "frames are saved under", save_data_directory)



