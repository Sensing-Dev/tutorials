/*

g++ src/tutorial5_parse_gendc_data.cpp -o tutorial5_parse_gendc_data  \
-I /opt/sensing-dev/include/opencv4 \
-I /opt/sensing-dev/include \
-lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc

*/

#include <fstream>
#include <filesystem>
#include <iostream>
#include <exception>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <regex>

#include "gendc_separator/ContainerHeader.h"
#include "gendc_separator/tools.h"

const std::regex number_pattern(R"(\d+)");

int extractNumber(const std::string& filename) {
    std::smatch match;
    std::regex_search(filename, match, number_pattern);
    return std::stoi(match[0]);
}

int main(int argc, char* argv[]){

    std::string directory_name = "tutorial_save_gendc_XXXXXXXXXXXXXX";
    int num_device = 1; // you may also get this number from config.json
    std::string prefix = "sensor0-";
    
    if (!std::filesystem::exists(directory_name)) {
        std::cerr << "Error: Directory '" << directory_name << "' does not exist.\n";
        return 1;
    }
    std::cout << "Directory: " << directory_name << std::endl;

    std::vector<std::string> bin_files;
    for (const auto& entry : std::filesystem::directory_iterator(directory_name)) {
        if (entry.path().filename().string().find(prefix) == 0 && entry.is_regular_file() && entry.path().extension() == ".bin") {
            bin_files.push_back(entry.path().filename().string());
        }
    }

    //re-order binary files to sensor0-0.bin, sensor0-1.bin, sensor0-2.bin...
    std::sort(bin_files.begin(), bin_files.end(), [](const std::string& a, const std::string& b) {
        return extractNumber(a) < extractNumber(b);
    });

    for (const auto& filename : bin_files){
        std::filesystem::path jth_bin= std::filesystem::path(directory_name) / std::filesystem::path(filename);

        std::ifstream ifs(jth_bin, std::ios::binary);
        if (!ifs.is_open()){
            throw std::runtime_error("Failed to open " + filename);
        }

        ifs.seekg(0, std::ios::end);
        std::streampos filesize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        char* filecontent = new char[filesize];

        if (!ifs.read(filecontent, filesize)) {
            delete[] filecontent;
            throw std::runtime_error("Failed to open " + filename);
        }

        int cursor = 0;

        // Check if the binary file has GenDC signature
        if (isGenDC(filecontent)){
            std::cout << " ues" << std::endl;
            while(cursor < static_cast<int>(filesize)){

                ContainerHeader gendc_descriptor= ContainerHeader(filecontent + cursor);

                // The following API to show all information of the container.
                //gendc_descriptor.DisplayHeaderInfo(); 
                
                // get GenDC container information
                int32_t descriptor_size = gendc_descriptor.getDescriptorSize();
                std::cout << "GenDC Descriptor size: " << descriptor_size << std::endl;
                int64_t data_size = gendc_descriptor.getContainerDataSize();
                std::cout << "GenDC Data size: " << data_size << std::endl;

                // get first available component
                #define ComponentIDIntensity 1
                // the other values: https://www.emva.org/wp-content/uploads/GenICam_SFNC_v2_7.pdf
                int32_t image_component_index = gendc_descriptor.getFirstComponentIndexWithDatatypeOf(ComponentIDIntensity);
                std::cout << "First available image data component is Comp " << image_component_index << std::endl;

                ComponentHeader image_component = gendc_descriptor.getComponentHeader(image_component_index);

                uint8_t* imagedata;
                imagedata = new uint8_t [image_component.getDataSize()];
                int32_t datasize = image_component.getData(reinterpret_cast<char*>(imagedata));

                std::vector<int32_t> image_dimension = image_component.getImageDimension();
                std::cout << "\tSize of image: ";
                int32_t WxHxC = 1;
                for (int i = 0; i < image_dimension.size(); ++i){
                    if (i > 0){
                        std::cout << "x";
                    }
                    std::cout << image_dimension[i];
                    WxHxC *= image_dimension[i];
                }
                std::cout << std::endl;

                // get byte-depth of pixel from data size and dimension
                int32_t bd = datasize / WxHxC;
                std::cout << "\tByte-depth of image: " << bd << std::endl;

                // Note that opencv mat type should be CV_<bit-depth>UC<channel num>
                cv::Mat img(image_dimension[1], image_dimension[0], CV_8UC1);
                std::memcpy(img.ptr(), imagedata, datasize);
                cv::imshow("First available image component", img);

                cv::waitKeyEx(1);

                // Access to Comp 0, Part 0's TypeSpecific 3 (where typespecific count start with 1)
                PartHeader partheader0 = image_component.getPartHeader(0);
                int offset = partheader0.getOffsetofTypeSpecific(3);
                // the following API is equivalent
                // offset = gendc_descriptor.getOffsetofTypeSpecific(0, 0, 3, 0);
                std::cout << "Framecount: " << *reinterpret_cast<uint32_t*>(filecontent + cursor + offset) << std::endl;
                
                cursor += (descriptor_size + data_size);
                std::cout << std::endl;

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
    }

    

    return 0; 
}