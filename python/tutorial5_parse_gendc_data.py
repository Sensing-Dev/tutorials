import re
import os

from  gendc_python.gendc_separator import descriptor as gendc

GDC_INTENSITY   = 0x0000000000000001

if __name__ == "__main__":

    directory_name = "tutorial_save_gendc_XXXXXXXXXXXXXX"
    prefix = "gendc0-"

    num_device = 1

    if not (os.path.exists(directory_name) and os.path.isdir(directory_name)):
        raise Exception("Directory" + directory_name + " does not exist")        

    bin_files = [f for f in os.listdir(directory_name) if f.startswith(prefix) and f.endswith(".bin")]
    bin_files = sorted(bin_files, key=lambda s: int(s.split('-')[-1].split('.')[0]))

    for bf in bin_files:
        bin_file = os.path.join(directory_name, bf)

        with open(bin_file, mode='rb') as ifs:
            filecontent = ifs.read()   
            cursor = 0

            while cursor < len(filecontent):
                try:
                    # TODO return NULL for non-gendc format
                    gendc_descriptor = gendc.Container(filecontent[cursor:])

                    # get GenDC container information
                    descriptor_size = gendc_descriptor.get("DescriptorSize")
                    print("GenDC Descriptor size:", descriptor_size)
                    data_size = gendc_descriptor.get("DataSize")
                    print("GenDC Data size:", data_size)

                    # get first available component
                    image_component = gendc_descriptor.get_first_get_datatype_of(GDC_INTENSITY)
                    print("First available image data component is Comp", image_component)
                    print("\tData size:", gendc_descriptor.get("DataSize", image_component, 0))
                    print("\tData offset:", gendc_descriptor.get("DataOffset", image_component, 0))

                    typespecific3 = gendc_descriptor.get("TypeSpecific", image_component, 0)[2]
                    print("Framecount: ", int.from_bytes(typespecific3.to_bytes(8, 'little')[0:4], "little"))

                    cursor = cursor + gendc_descriptor.get_container_size()
                    print()

                except:
                    print("========================================")
                    print("This is not GenDC Format data.")
                    print("If you save this with image_io_binarysaver_u{}x{} BB, the data structure is")
                    print()
                    print("| framecount0 | imagedata0 | framecount1 | imagedata1 | framecount2 | imagedata2 | ... |")
                    print()
                    print("framecount is 4 byte-length, and imagedata size is width * height * byte-depth * number of channel, ")
                    print("which you may be able to find/calculate from config.json file.")
                    print("Note that framecount is not frame id. Some device may not have this number, and if so, it is filled with 0.")
                    print("========================================")
                    raise Exception("This is not GenDC Format")

