/*

g++ src/parse_gendc_data_dummy.cpp -o parse_gendc_data_dummy \
-I /opt/sensing-dev/include/opencv4 \
-I /opt/sensing-dev/include \
-L /opt/sensing-dev/lib \
-L /opt/sensing-dev/lib/x86_64-linux-gnu \
-ldl -lpthread -lopencv_core \
-lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc

*/


#include <fstream>
#include <filesystem>
#include <iostream>
#include <exception>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>


#include "gendc_separator/ContainerHeader.h"
#include "gendc_separator/tools.h"

#define ComponentIDIntensity 1

#define Mono8 0x01080001
#define Mono10 0x01100003
#define Mono12 0x01100005
#define RGB8 0x02180014
#define BGR8 0x02180015

int positive_pow(int base, int expo){
  if (expo <= 0){
      return 1;
  }
  if (expo == 1){
      return base;
  }else{
      return base * positive_pow(base, expo-1);
  }
}

int getBitShift(int pfnc_pixelformat){
    if (pfnc_pixelformat == Mono8 || pfnc_pixelformat == RGB8 || pfnc_pixelformat == BGR8){
        return 0;
    }else if (pfnc_pixelformat == Mono10){
        return 6;
    }
    else if (pfnc_pixelformat == Mono12){
        return 4;
    }else{
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << pfnc_pixelformat << " is not supported as default in this tutorial.\nPlease update getBitShift()";
        std::string hexString = ss.str();
        throw std::runtime_error(hexString);
    }
}

int getCVMatType(int byte_depth, std::vector<int32_t>& image_dimension){
    if (image_dimension.size() == 3){
        if (image_dimension[2] == 3 && byte_depth == 1){
            return CV_8UC3;
        }
    }else if (image_dimension.size() == 2){
        if (byte_depth == 1){
            return CV_8UC1;
        }else if (byte_depth == 2){
            return CV_16UC1;
        }
    }
    std::stringstream ss;
    ss << "This is not supported as default in this tutorial.\nPlease update getCVMatType()";
    std::string hexString = ss.str();
    throw std::runtime_error(hexString);
}

int extractNumber(const std::string& filename) {
    size_t dashPos = filename.rfind('-');
    size_t dotPos = filename.rfind('.');
    if (dashPos == std::string::npos || dotPos == std::string::npos || dashPos >= dotPos)
        return -1;  // Return -1 or throw an exception if the format is not as expected

    std::string number = filename.substr(dashPos + 1, dotPos - dashPos - 1);
    return std::stoi(number);
}

int main(int argc, char* argv[]){

    std::string directory_name = ".";
    bool user_dummy_data = false;
    std::string prefix = "gendc0-";

    if (argc > 1){
        for (int i = 1; i < argc; i++){
            if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--directory") == 0){
                directory_name = argv[++i];
            } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--use-dummy-data") == 0){
                user_dummy_data = true;
            }
        }
    }

    if (!std::filesystem::exists(directory_name)) {
        std::cerr << "Error: Directory '" << directory_name << "' does not exist.\n";
        return 1;
    }
    std::cout << "Directory: " << directory_name << std::endl;

    std::vector<std::string> bin_files;
    if (user_dummy_data){
        std::filesystem::path sample_data_file = std::filesystem::path(directory_name) / std::filesystem::path("output.bin");
        bin_files.push_back("output.bin");
        if (!std::filesystem::exists(sample_data_file)) {
            std::cerr << "Error: Sample data output.bin does not exist under " << directory_name << "/ .\n";
            return 1;
        }
    }else{
        for (const auto& entry : std::filesystem::directory_iterator(directory_name)) {
            if (entry.path().filename().string().find(prefix) == 0 && entry.is_regular_file() && entry.path().extension() == ".bin") {
                bin_files.push_back(entry.path().filename().string());
            }
        }
        if (bin_files.size() == 0){
            std::cout << "no detect bin files with prefix " << prefix << " detected" <<std::endl;
        }

        //re-order binary files to sensor0-0.bin, sensor0-1.bin, sensor0-2.bin...
        std::sort(bin_files.begin(), bin_files.end(), [](const std::string& a, const std::string& b) {
            return extractNumber(a) < extractNumber(b);
        });
    }

    for (const auto& filename : bin_files) {
        std::filesystem::path jth_bin = std::filesystem::path(directory_name) / std::filesystem::path(filename);

        std::ifstream ifs(jth_bin, std::ios::binary);
        if (!ifs.is_open()) {
            throw std::runtime_error("Failed to open " + filename);
        }

        ifs.seekg(0, std::ios::end);
        std::streampos filesize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        char *filecontent = new char[filesize];

        if (!ifs.read(filecontent, filesize)) {
            delete[] filecontent;
            throw std::runtime_error("Failed to open " + filename);
        }

        int32_t cursor = 0;
        bool first_container = true;

        // Check if the binary file has GenDC signature
        if (isGenDC(filecontent)) {
            while (cursor < static_cast<int>(filesize)) {

                ContainerHeader gendc_descriptor = ContainerHeader(filecontent + cursor);
                // The following API to show all information of the container.
                //gendc_descriptor.DisplayHeaderInfo();

                // get GenDC container information
                int32_t descriptor_size = gendc_descriptor.getDescriptorSize();
                std::cout << "GenDC Descriptor size: " << descriptor_size << std::endl;
                int64_t container_data_size = gendc_descriptor.getDataSize();
                std::cout << "GenDC Data size: " << container_data_size << std::endl;

                // get first available image component

                // the other values: https://www.emva.org/wp-content/uploads/GenICam_SFNC_v2_7.pdf
                int32_t image_component_index = gendc_descriptor.getFirstComponentIndexByTypeID(ComponentIDIntensity);
                if (image_component_index != -1){
                    ComponentHeader image_component = gendc_descriptor.getComponentByIndex(image_component_index);
                    std::cout << "First available image data component is Comp " << image_component_index << std::endl;

                    // Get PixelFormat
                    // API to get PixelFormat would be added in the future
                    int32_t image_component_header_offset = *reinterpret_cast<int32_t *>(filecontent + cursor + 56 + 8 * image_component_index);
                    int32_t pfnc_pixelformat = *reinterpret_cast<int32_t *>(filecontent + cursor + image_component_header_offset + 40);
                    int32_t num_bitshift = getBitShift(pfnc_pixelformat);

                    int part_count = image_component.getPartCount();
                    std::cout << "\tData Channel: " << part_count << std::endl;

                    int64_t part_data_cursor = cursor + descriptor_size;
                    for (int idx = 0; idx < part_count; idx++) {
                        PartHeader part = image_component.getPartByIndex(idx);
                        uint8_t *imagedata;
                        int64_t part_data_size = part.getDataSize();
                        imagedata = new uint8_t[part_data_size];
                        part.getData(reinterpret_cast<char *>(imagedata));

                        std::vector <int32_t> image_dimension = part.getDimension();
                        std::cout << "\tDimension: ";
                        int32_t WxHxC = 1;
                        for (int i = 0; i < image_dimension.size(); ++i) {
                            if (i > 0) {
                                std::cout << "x";
                            }
                            std::cout << image_dimension[i];
                            WxHxC *= image_dimension[i];
                        }
                        std::cout << std::endl;
                        // get byte-depth of pixel from data size and dimension
                        int32_t bd = static_cast<int32_t>(part_data_size / WxHxC);
                        std::cout << "\tByte-depth: " << bd << std::endl;

                        // Note that opencv mat type should be CV_<bit-depth>UC<channel num>
                        cv::Mat img(image_dimension[1], image_dimension[0], getCVMatType(bd, image_dimension));
                        std::memcpy(img.ptr(), imagedata, part_data_size);
                        img = img * pow(2, num_bitshift);

//                        cv::imshow("First available image component", img);
//
//                        if (user_dummy_data){
//                            cv::waitKeyEx(0);
//                        }else{
//                            cv::waitKeyEx(1);
//                        }

                // Access to Comp 0, Part 0's TypeSpecific 3 (where typespecific count start with 1; therefore, index is 2)
                        int64_t typespecific3 = part.getTypeSpecificByIndex(2);
                        // Access to the first 4-byte of typespecific3
                        int32_t framecount = static_cast<int32_t>(typespecific3 & 0xFFFFFFFF);

                        std::cout << "Framecount: " << framecount<< std::endl;
                        part_data_cursor +=  part_data_size;

                        delete[] imagedata;
                    }
                }else{
                    if (first_container){
                        std::cout << "Skip Image Component - This GenDC does not have component of typeId " << ComponentIDIntensity << std::endl;
                    }
                }

                int32_t audio_sourceId = 0x2001;
                int audio_component_index = gendc_descriptor.getFirstComponentIndexBySourceId(audio_sourceId);
                if (audio_component_index != -1){
                    std::cout << "First available audio data component is Comp " << audio_component_index << std::endl;
                    ComponentHeader audio_component = gendc_descriptor.getComponentByIndex(audio_component_index);

                    int audio_part_count = audio_component.getPartCount();
                    std::cout << "\tData Channel: " << audio_part_count << std::endl;

                    for (int idx = 0; idx < audio_part_count; idx++) {
                        PartHeader part = audio_component.getPartByIndex(idx);
                        int64_t part_data_size = part.getDataSize();

                        std::vector <int32_t> audio_dimension = part.getDimension();
                        std::cout << "\tDimension: ";
                        int32_t WxHxC = 1;
                        for (int i = 0; i < audio_dimension.size(); ++i) {
                            if (i > 0) {
                                std::cout << "x";
                            }
                            std::cout << audio_dimension[i];
                            WxHxC *= audio_dimension[i];
                        }
                        std::cout << std::endl;
                        // get byte-depth of pixel from data size and dimension
                        int32_t bd = static_cast<int32_t>(part_data_size / WxHxC);
                        std::cout << "\tByte-depth: " << bd << std::endl;
                    }
                }else{
                    if (first_container){
                        std::cout << "Skip Audio Component - This GenDC does not have component of sourceId " << audio_sourceId << std::endl;
                    }
                }

                int32_t analog_sourceId = 0x3001;
                int analog_component_index = gendc_descriptor.getFirstComponentIndexBySourceId(analog_sourceId);
                if (analog_component_index != -1){
                    std::cout << "First available analog data component is Comp " << analog_component_index << std::endl;
                    ComponentHeader analog_component = gendc_descriptor.getComponentByIndex(analog_component_index);

                    int analog_part_count = analog_component.getPartCount();
                    std::cout << "\tData Channel: " << analog_part_count << std::endl;

                    for (int idx = 0; idx < analog_part_count; idx++) {
                        PartHeader part = analog_component.getPartByIndex(idx);
                        int64_t part_data_size = part.getDataSize();

                        std::vector <int32_t> analog_dimension = part.getDimension();
                        std::cout << "\tDimension: ";
                        int32_t WxHxC = 1;
                        for (int i = 0; i < analog_dimension.size(); ++i) {
                            if (i > 0) {
                                std::cout << "x";
                            }
                            std::cout << analog_dimension[i];
                            WxHxC *= analog_dimension[i];
                        }
                        std::cout << std::endl;
                        // get byte-depth of pixel from data size and dimension
                        int32_t bd = static_cast<int32_t>(part_data_size / WxHxC);
                        std::cout << "\tByte-depth: " << bd << std::endl;
                    }
                }else{
                    if (first_container){
                        std::cout << "Skip Analog Component - This GenDC does not have component of sourceId " << analog_sourceId << std::endl;
                    }
                }

                int32_t pmod_sourceId = 0x4001;
                int pmod_component_index = gendc_descriptor.getFirstComponentIndexBySourceId(pmod_sourceId);
                if (pmod_component_index != -1){
                    std::cout << "First available pmod data component is Comp " << pmod_component_index << std::endl;
                    ComponentHeader pmod_component = gendc_descriptor.getComponentByIndex(pmod_component_index);

                    int pmod_part_count = pmod_component.getPartCount();
                    std::cout << "\tData Channel: " << pmod_part_count << std::endl;

                    for (int idx = 0; idx < pmod_part_count; idx++) {
                        PartHeader part = pmod_component.getPartByIndex(idx);
                        int64_t part_data_size = part.getDataSize();

                        std::vector <int32_t> pmod_dimension = part.getDimension();
                        std::cout << "\tDimension: ";
                        int32_t WxHxC = 1;
                        for (int i = 0; i < pmod_dimension.size(); ++i) {
                            if (i > 0) {
                                std::cout << "x";
                            }
                            std::cout << pmod_dimension[i];
                            WxHxC *= pmod_dimension[i];
                        }
                        std::cout << std::endl;
                        // get byte-depth of pixel from data size and dimension
                        int32_t bd = static_cast<int32_t>(part_data_size / WxHxC);
                        std::cout << "\tByte-depth: " << bd << std::endl;
                    }
                }else{
                    if (first_container){
                        std::cout << "Skip PMOD Component - This GenDC does not have component of sourceId " << pmod_sourceId << std::endl;
                    }
                }

                cursor += static_cast<int32_t>(descriptor_size + container_data_size);
                first_container = false;
            }
        }else{
            std::cout << "This is not GenDC Format data.\n" <<
                "If you save this with image_io_binarysaver_u{}x{} BB, the data structure is\n" <<
                "\n" <<
                "| framecount0 | imagedata0 | framecount1 | imagedata1 | framecount2 | imagedata2 | ... |\n" <<
                "\n" <<
                "framecount is 4 byte-length, and imagedata size is width * height * byte-depth * number of channel, " <<
                "which you may be able to find/calculate from config.json file." << std::endl;
                std::cout << "Note that framecount is not frame id. Some device may not have this number, and if so, it is filled with 0." << std::endl;
            throw std::runtime_error("This is not GenDC Format");
        }
        delete[] filecontent;
    }

    return 0;
}
