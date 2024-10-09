import os
import numpy as np


import json

import struct

Mono8 = 0x01080001
Mono10 = 0x01100003
Mono12 = 0x01100005
RGB8 = 0x02180014
BGR8 = 0x02180015

if __name__ == "__main__":

    directory_name = "."
    prefix = "image0-"

    num_device = 1

    if not (os.path.exists(directory_name) and os.path.isdir(directory_name)):
        raise Exception("Directory " + directory_name + " does not exist")

    # image info from image0-config.json
    f = open(os.path.join(directory_name, prefix + "config.json"))
    config = json.loads(f.read())
    f.close()

    w = config["width"]
    h = config["height"]
    pixelformat = config["pfnc_pixelformat"]
    d = 2 if config["pfnc_pixelformat"] == Mono10 or config["pfnc_pixelformat"] == Mono12 \
        else 1
    c = 3 if config["pfnc_pixelformat"] == RGB8 or config["pfnc_pixelformat"] == BGR8 \
        else 1
    framesize = w * h * d * c

    np_dtype = np.uint8 if d == 1 else np.uint16

    num_bit_shift = 0 if pixelformat == Mono8 or pixelformat == RGB8 or pixelformat == BGR8 \
        else 4 if pixelformat == Mono12 \
        else 6 if pixelformat == Mono10 \
        else 0
    coef = pow(2, num_bit_shift)

    bin_files = [f for f in os.listdir(directory_name) if f.startswith(prefix) and f.endswith(".bin")]
    bin_files = sorted(bin_files, key=lambda s: int(s.split('-')[-1].split('.')[0]))
    if len(bin_files) == 0:
        raise Exception("No bin files with prefix {}  detected".format(prefix))
    for bf in bin_files:
        bin_file = os.path.join(directory_name, bf)

        with open(bin_file, mode='rb') as ifs:
            filecontent = ifs.read()
            cursor = 0

            while cursor < len(filecontent):
                framecount = struct.unpack('I', filecontent[cursor:cursor + 4])[0]
                image = np.frombuffer(filecontent[cursor + 4:cursor + 4 + framesize], dtype=np_dtype).reshape(
                    (h, w)).copy()
                image *= coef

                print(framecount)

                cursor = cursor + 4 + framesize


