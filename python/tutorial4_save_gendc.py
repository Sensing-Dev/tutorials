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
    # gain_key = Param('gain_key', 'Gain')
    # exposure_key = Param('exposure_key', 'ExposureTime')

    # add a node to pipeline
    node = builder.add("image_io_u3v_gendc")\
        .set_param([num_devices, frame_sync, realtime_diaplay_mode, ])

    node = builder.add("image_io_binary_gendc_saver")\
        .set_iport([node.get_port('gendc'), node.get_port('device_info'), payloadsize_p, ])\
        .set_param([num_devices, output_directory, 
                    Param('input_gendc.size', str(num_device)), 
                    Param('input_deviceinfo.size', str(num_device)) ])

    # create halide buffer for output port
    terminator = node.get_port('output')
    output = Buffer(Type(TypeCode.Int, 32, 1), ())
    terminator.bind(output)

    # bind input values to the input port
    payloadsize_p.bind(payloadsize)

    num_run = 0

    print("Hi any key to stop saving")
    
    try:
        while True:
            builder.run()
            num_run += 1
    except KeyboardInterrupt:
        pass

    print( num_run, "frames are saved under", save_data_directory)



