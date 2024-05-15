import re
import os

import numpy as np
import cv2

from gendc_python.gendc_separator import descriptor as gendc

GDC_INTENSITY = 1

if __name__ == "__main__":

    directory_name = "tutorial_save_gendc_xxxxxxxxxxxx"
    prefix = "gendc0-"

    num_device = 1

    if not (os.path.exists(directory_name) and os.path.isdir(directory_name)):
        raise Exception("Directory" + directory_name + " does not exist")

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

                # TODO return NULL for non-gendc format
                gendc_container = gendc.Container(filecontent[cursor:])

                # get GenDC container information
                descriptor_size = gendc_container.get_descriptor_size()
                print("GenDC Descriptor size:", descriptor_size)
                data_size = gendc_container.get_data_size()
                print("GenDC Data size:", data_size)

                # get first available component
                image_component_idx = gendc_container.get_1st_component_idx_by_typeid(GDC_INTENSITY)
                print("First available image data component is Comp", image_component_idx)
                image_component = gendc_container.get_component_by_idx(image_component_idx)
                part_count = image_component.get_part_count()
                print("\tData Channel: ", part_count)

                for part_id in range(part_count):
                    part = image_component.get_part_by_idx(part_id)
                    dimension = part.get_dimension()
                    print("\tDimension: ", "x".join(str(v) for v in dimension))
                    w_h_c = part_count
                    for d in dimension:
                        w_h_c *= d
                    byte_depth = int(data_size / w_h_c)
                    print("\tByte-depth of image", byte_depth)
                    typespecific3 = part.get("TypeSpecific")[2]
                    print("Frame count: ", int.from_bytes(typespecific3.to_bytes(8, 'little')[0:4], "little"))
                    cursor = cursor + gendc_container.get_container_size()
                    width = dimension[0]
                    height = dimension[1]
                    if byte_depth == 1:
                        image = np.frombuffer(part.get_data(), dtype=np.uint8).reshape((height, width))
                    elif byte_depth == 2:
                        image = np.frombuffer(part.get_data(), dtype=np.uint16).reshape((height, width))
                    user_input = -1
                    cv2.imshow("img", image)
                    cv2.waitKey(1)
            cv2.destroyAllWindows()
