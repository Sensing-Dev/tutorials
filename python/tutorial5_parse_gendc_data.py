import argparse
import os

import numpy as np
import cv2

from gendc_python.gendc_separator import descriptor as gendc
from gendc_python.genicam import tool as genicam

GDC_INTENSITY = 1

Mono8 = 0x01080001
Mono10 = 0x01100003
Mono12 = 0x01100005
RGB8 = 0x02180014
BGR8 = 0x02180015

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Tutorial: Parse GenDC Data")
    parser.add_argument('-d', '--directory', default='.', type=str, \
                        help='Directory that has bin files')
    parser.add_argument('-u', '--use-dummy-data', \
                        action='store_true', help='Use Dummy data downloaded from Sensing-Dev/GenDC.')
    parser.add_argument('-q', '--quiet', \
                        action='store_true', help='Disable image display')

    directory_name = parser.parse_args().directory
    use_dummy_data = parser.parse_args().use_dummy_data
    image_display = not parser.parse_args().quiet
    prefix = "gendc0-"

    num_device = 1

    if not (os.path.exists(directory_name) and os.path.isdir(directory_name)):
        raise Exception("Directory " + directory_name + " does not exist")
    
    if use_dummy_data:
        bin_files = ['output.bin']
        if not (os.path.isfile(os.path.join(directory_name, 'output.bin'))):
            raise Exception("dummy data output.bin not found in " + directory_name + '\nPlease Download from https://github.com/Sensing-Dev/GenDC/blob/main/test/generated_stub/output.bin')
    else:
        bin_files = [f for f in os.listdir(directory_name) if f.startswith(prefix) and f.endswith(".bin")]
        bin_files = sorted(bin_files, key=lambda s: int(s.split('-')[-1].split('.')[0]))

        if len(bin_files) == 0:
            raise Exception("No bin files with prefix {}  detected".format(prefix))
        
    for bf in bin_files:
        bin_file = os.path.join(directory_name, bf)

        with open(bin_file, mode='rb') as ifs:
            filecontent = ifs.read()
            cursor = 0
            first_container = True

            while cursor < len(filecontent):

                # TODO return NULL for non-gendc format
                gendc_container = gendc.Container(filecontent[cursor:])

                # get GenDC container information
                descriptor_size = gendc_container.get_descriptor_size()
                print("GenDC Descriptor size:", descriptor_size)
                container_data_size = gendc_container.get_data_size()
                print("GenDC Data size:", container_data_size)

                # get first available image component
                image_component_idx = gendc_container.get_1st_component_idx_by_typeid(GDC_INTENSITY)
                if image_component_idx != -1:
                    print("First available image data component is Component", image_component_idx)
                    image_component = gendc_container.get_component_by_index(image_component_idx)

                    pfnc_pixelformat = image_component.get('Format')
                    num_bit_shift = 0 if pfnc_pixelformat == Mono8 or pfnc_pixelformat == RGB8 or  pfnc_pixelformat == BGR8 \
                        else 4 if pfnc_pixelformat == Mono12 \
                        else 6 if pfnc_pixelformat == Mono10 \
                        else 0
                    coef = pow(2, num_bit_shift)

                    part_count = image_component.get_part_count()
                    print("\tData Channel: ", part_count)
                    cursor = cursor + descriptor_size
                    for part_id in range(part_count):
                        part = image_component.get_part_by_index(part_id)
                        part_data_size =part.get_data_size()
                        dimension = part.get_dimension()
                        print("\tDimension: ", "x".join(str(v) for v in dimension))
                        w_h_c = 1
                        for d in dimension:
                            w_h_c *= d
                        byte_depth = int(part_data_size / w_h_c)
                        print("\tByte-depth of image", byte_depth)

                        # Access to Comp 0, Part 0's TypeSpecific 3 (where typespecific count start with 1; therefore, index is 2)
                        typespecific3 = part.get_typespecific_by_index(2)
                        # Access to the first 4-byte of typespecific3
                        print("Framecount: ", int.from_bytes(typespecific3.to_bytes(8, 'little')[0:4], "little")) 

                        if image_display:
                            width = dimension[0]
                            height = dimension[1]
                            if byte_depth == 1:
                                image = np.frombuffer(part.get_data(), dtype=np.uint8).reshape((height, width)).copy()
                            elif byte_depth == 2:
                                image = np.frombuffer(part.get_data(), dtype=np.uint16).reshape((height, width)).copy()
                            image *= coef
                            cv2.imshow("First available image component", image)
                            if use_dummy_data:
                                cv2.waitKey(0)
                            else:
                                cv2.waitKey(1)
                else:
                    if first_container:
                        print("Skip Image Component - This GenDC does not have component of typeId", GDC_INTENSITY)
                
                audio_component_index = gendc_container.get_1st_component_idx_by_sourceid(0x2001)
                if audio_component_index != -1:
                    print("First available audio data component is Component", audio_component_index)
                    audio_component = gendc_container.get_component_by_index(audio_component_index)

                    part_count = audio_component.get_part_count()
                    print("\tData Channel: ", part_count)
                    for j in range(part_count):
                        part = audio_component.get_part_by_index(j)
                        part_data_size =part.get_data_size()
                        dimension = part.get_dimension()
                        print("\tDimension: ", "x".join(str(v) for v in dimension))
                        w_h_c = 1
                        for d in dimension:
                            w_h_c *= d
                        byte_depth = int(part_data_size / w_h_c)
                        print("\tByte-depth of image", byte_depth)
                else:
                    if first_container:
                        print("Skip Audio Component - This GenDC does not have component of sourceId", 0x2001)

                analog_component_index = gendc_container.get_1st_component_idx_by_sourceid(0x3001)
                if analog_component_index != -1:
                    print("First available analog data component is Component", analog_component_index)
                    analog_component = gendc_container.get_component_by_index(analog_component_index)

                    part_count = analog_component.get_part_count()
                    print("\tData Channel: ", part_count)
                    for j in range(part_count):
                        part = analog_component.get_part_by_index(j)
                        part_data_size =part.get_data_size()
                        dimension = part.get_dimension()
                        print("\tDimension: ", "x".join(str(v) for v in dimension))
                        w_h_c = 1
                        for d in dimension:
                            w_h_c *= d
                        byte_depth = int(part_data_size / w_h_c)
                        print("\tByte-depth of image", byte_depth)
                else:
                    if first_container:
                        print("Skip Analog Component - This GenDC does not have component of sourceId", 0x3001)

                pmod_component_index = gendc_container.get_1st_component_idx_by_sourceid(0x4001)
                if pmod_component_index != -1:
                    print("First available PMOD data component is Component", pmod_component_index)
                    pmog_component = gendc_container.get_component_by_index(pmod_component_index)

                    part_count = pmog_component.get_part_count()
                    print("\tData Channel: ", part_count)
                    for j in range(part_count):
                        part = pmog_component.get_part_by_index(j)
                        part_data_size =part.get_data_size()
                        dimension = part.get_dimension()
                        print("\tDimension: ", "x".join(str(v) for v in dimension))
                        w_h_c = 1
                        for d in dimension:
                            w_h_c *= d
                        byte_depth = int(part_data_size / w_h_c)
                        print("\tByte-depth of image", byte_depth)
                else:
                    if first_container:
                        print("Skip PMOD Component - This GenDC does not have component of sourceId", 0x4001)

                cursor = cursor + container_data_size
                first_container = False
            cv2.destroyAllWindows()
