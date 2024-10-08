import numpy as np
from ionpy import Node, Builder, Buffer, Port, Param, Type, TypeCode

if __name__ == "__main__":

    # device information
    width = 640
    height = 480
    pixelformat = "Mono8"
    num_device = 2

    # the following items varies by PixelFormat
    data_type = np.uint8 if pixelformat == "Mono8" \
        else np.uint16 if pixelformat == "Mono16" \
        else np.uint8
    depth_of_buffer = np.iinfo(data_type).bits
    depth_in_byte = int(depth_of_buffer / 8)
    bb_name = "image_io_u3v_cameraN_u8x2" if pixelformat == "Mono8" \
        else "image_io_u3v_cameraN_u16x2" if pixelformat == "Mono16" \
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
    node = builder.add(bb_name) \
        .set_params([num_devices, frame_sync, realtime_display_mode,
                     Param('width', width),
                     Param('height', height)])
    output_p = node.get_port('output')

    # create halide buffer for output port
    outputs = []
    output_datas = []
    output_size = (height, width,)

    for i in range(num_device):
        output_datas.append(np.full(output_size, fill_value=0, dtype=data_type))
        outputs.append(Buffer(array=output_datas[i]))

    # set I/O ports
    output_p.bind(outputs)

    for i in range(10):
        # running the builder
        builder.run()
        print(output_datas[0])
        print(output_datas[1])



