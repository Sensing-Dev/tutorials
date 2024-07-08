import argparse
import os

import numpy as np
import matplotlib.pyplot as plt

from gendc_python.gendc_separator import descriptor as gendc

GDC_INTENSITY = 1

Mono8 = 0x01080001
Mono10 = 0x01100003
Mono12 = 0x01100005
RGB8 = 0x02180014
BGR8 = 0x02180015

def get_numpy_dtype(byte_depth, unsinged=False):
    if byte_depth == 1:
        return np.uint8 if unsinged else np.int8
    elif byte_depth == 2:
        return np.uint16 if unsinged else np.int16
    elif byte_depth == 2:
        return np.uint16 if unsinged else np.int16
    elif byte_depth == 2:
        return np.uint16 if unsinged else np.int16
    else:
        raise Exception('Byte-depth ' + str(byte_depth) + ' is not supported')
    
if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Tutorial: Parse GenDC Data")
    parser.add_argument('-d', '--directory', default='.', type=str, \
                        help='Directory that has bin files')
    parser.add_argument('-f', '--framerate', default=60, type=int, \
                        help='Framerate of the recorded GenDC image sensor')
    directory_name = parser.parse_args().directory
    image_fps = parser.parse_args().framerate

    if not (os.path.exists(directory_name) and os.path.isdir(directory_name)):
        raise Exception("Directory " + directory_name + " does not exist")
    

    bin_file = os.path.join(directory_name, 'output.bin')
    if not (os.path.isfile(bin_file)):
        raise Exception("dummy data output.bin not found in " + directory_name)
        

    with open(bin_file, mode='rb') as ifs:
        filecontent = ifs.read()

        # GenDC Container information
        gendc_container = gendc.Container(filecontent)

        descriptor_size = gendc_container.get_descriptor_size()
        print("GenDC Descriptor size:", descriptor_size)
        container_data_size = gendc_container.get_data_size()
        print("GenDC Data size:", container_data_size)

        # imge component #######################################################
        image_component_idx = gendc_container.get_1st_component_idx_by_sourceid(0x1001)
        print("Image data component is Component", image_component_idx)
        image_component = gendc_container.get_component_by_index(image_component_idx)

        part_count = image_component.get_part_count()

        for part_id in range(part_count):
            part = image_component.get_part_by_index(part_id)
            part_data_size =part.get_data_size()
            dimension = part.get_dimension()
            w_h_c = np.prod(dimension)
            byte_depth = int(part_data_size / w_h_c)

            # Access to Comp 0, Part 0's TypeSpecific 3 (where typespecific count start with 1; therefore, index is 2)
            typespecific3 = part.get_typespecific_by_index(2)
            # Access to the first 4-byte of typespecific3
            frame_count_of_this_container = int.from_bytes(typespecific3.to_bytes(8, 'little')[0:4], "little")

            image_data = np.frombuffer(part.get_data(), dtype=get_numpy_dtype(byte_depth, unsinged=True)).reshape(dimension[::-1])

            image_fig = plt.figure(figsize=(15, 5))
            plt.imshow(image_data, cmap='gist_gray')

    # audio component ##########################################################
    component_index = gendc_container.get_1st_component_idx_by_sourceid(0x2001)
    print("Aduio data component is Component", component_index)
    audio_component = gendc_container.get_component_by_index(component_index)

    for j in range(audio_component.get_part_count()):
        part = audio_component.get_part_by_index(j)
        part_data_size =part.get_data_size()
        dimension = part.get_dimension()
        w_h_c = np.prod(dimension)
        byte_depth = int(part_data_size / w_h_c)

        # audio specific #######################################
        left_and_right_channel = 2
        titles = ['Audio L ch', 'Audio R ch']
        ylabels = ['Audio L ch Amplitude', 'Audio R ch Amplitude']
        ########################################################

        dimension.append(left_and_right_channel)
        byte_depth = int(byte_depth / left_and_right_channel)
        
        audio_part_data = np.frombuffer(part.get_data(), dtype=get_numpy_dtype(byte_depth, unsinged=False)).reshape(dimension)
        num_samples, num_data = audio_part_data.shape
        times = np.linspace(0, num_samples/(image_fps * num_samples), num=num_samples)[:num_samples]
    
        audio_fig = plt.figure(figsize=(15, 5))
        for kth_plot in range(num_data):
            ax = audio_fig.add_subplot(num_data, 1, kth_plot+1)
            ax.plot(times, audio_part_data[:, kth_plot])
            ax.title.set_text(titles[kth_plot])
            ax.set_xlabel("time [s]")
            ax.set_ylabel("Amplitude")
        plt.tight_layout()
        
        # analog data ##########################################################
        component_index = gendc_container.get_1st_component_idx_by_sourceid(0x3001)
        print("Analog data component is Component", component_index)
        analog_component = gendc_container.get_component_by_index(component_index)

        part_count = analog_component.get_part_count()
        for j in range(part_count):
            part = analog_component.get_part_by_index(j)
            part_data_size =part.get_data_size()
            dimension = part.get_dimension()
            w_h_c = np.prod(dimension)
            byte_depth = int(part_data_size / w_h_c)

            # analog specific ######################################
            titles = ['Analog']
            num_channel = 1
            ########################################################

            dimension.append(num_channel)
            byte_depth = int(byte_depth / num_channel)
            
            analog_part_data = np.frombuffer(part.get_data(), dtype=get_numpy_dtype(byte_depth, unsinged=False)).reshape(dimension)
            num_samples, num_data = analog_part_data.shape
            times = np.linspace(0, num_samples/(image_fps * num_samples), num=num_samples)[:num_samples]

            analog_fig = plt.figure(figsize=(15, 5))
            plt.plot(times, analog_part_data, marker = 'o')
            plt.title('Analog data')
            plt.xlabel("time [s]")
            plt.ylabel("Amplitude")
            plt.tight_layout()

        # PMOD data ##############################################
        component_index = gendc_container.get_1st_component_idx_by_sourceid(0x4001)
        print("PMOD data component is Component", component_index)
        pmog_component = gendc_container.get_component_by_index(component_index)

        for j in range(pmog_component.get_part_count()):
            part = pmog_component.get_part_by_index(j)
            part_data_size =part.get_data_size()
            dimension = part.get_dimension()
            w_h_c = np.prod(dimension)
            byte_depth = int(part_data_size / w_h_c)

            # PMOD specific ######################################
            xyz_coordinate_num = 4 #(x, y, z, _)
            titles = ['PMOD x', 'PMOD y', 'PMOD z']
            ########################################################

            dimension.append(xyz_coordinate_num)
            byte_depth = int(byte_depth / xyz_coordinate_num)

            pmod_part_data = np.frombuffer(part.get_data(), dtype=get_numpy_dtype(byte_depth, unsinged=False)).reshape(dimension)
            num_samples, num_data = pmod_part_data.shape
            times = np.linspace(0, num_samples/(image_fps * num_samples), num=num_samples)[:num_samples]

            pmod_fig = plt.figure(figsize=(15, 10))
            pmod_fig.tight_layout()
            
            ax = pmod_fig.add_subplot(projection='3d')
            ax.plot(pmod_part_data[:, 0], pmod_part_data[:, 1], pmod_part_data[:, 2], marker = 'o')
            ax.title.set_text(titles[kth_plot])
            ax.set_xlabel("x")
            ax.set_ylabel("y")
            ax.set_zlabel("z")
            plt.tight_layout()
    
        plt.show()


