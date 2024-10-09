/*

g++ src/tutorial5_parse_image_data.cpp -o tutorial5_parse_image_data \
-I /opt/sensing-dev/include/opencv4 \
-I /opt/sensing-dev/include \
-I src \
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


#include <json/json.hpp>

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


int getByteDepth(int pfnc_pixelformat){
    if (pfnc_pixelformat == Mono8 || pfnc_pixelformat == RGB8 || pfnc_pixelformat == BGR8){
        return 1;
    }else if (pfnc_pixelformat == Mono10 || pfnc_pixelformat == Mono12){
        return 2;
    }else{
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << pfnc_pixelformat << " is not supported as default in this tutorial.\nPlease update getByteDepth()";
        std::string hexString = ss.str();
        throw std::runtime_error(hexString);
    }
}

int getNumChannel(int pfnc_pixelformat){
    if (pfnc_pixelformat == Mono8 || pfnc_pixelformat == Mono10 || pfnc_pixelformat == Mono12){
        return 1;
    }else if (pfnc_pixelformat == RGB8 || pfnc_pixelformat == BGR8){
        return 3;
    }else{
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << pfnc_pixelformat << " is not supported as default in this tutorial.\nPlease update getNumChannel()";
        std::string hexString = ss.str();
        throw std::runtime_error(hexString);
    }
}


int getOpenCVMatType(int d, int c){
    if (d == 2){
        if (c == 1){
            return CV_16UC1;
        }else if (c == 3){
            return CV_16UC3;
        }
    }else if (d ==1){
     if (c == 1){
            return CV_8UC1;
        }else if (c == 3){
            return CV_8UC3;
        }
    }
    std::stringstream ss;
    ss << "byte-depth " << d << ", channel num " << c << " image is not supported as default in this tutorial.\nPlease update getOpenCVMatType()";
    throw std::runtime_error(ss.str());
    return -1;
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

    std::string directory_name = "tutorial_save_image_bin_XXXXXXXXXXXXXXXXXX";
    std::string prefix = "image0-";

    if (!std::filesystem::exists(directory_name)) {
        std::cerr << "Error: Directory '" << directory_name << "' does not exist.\n";
        return 1;
    }
    std::cout << "Directory: " << directory_name << std::endl;

    std::ifstream f(std::filesystem::path(directory_name) / std::filesystem::path(prefix+"config.json"));
    nlohmann::json config = nlohmann::json::parse(f);

    int32_t w = config["width"];
    int32_t h = config["height"];
    int32_t d = getByteDepth(config["pfnc_pixelformat"]);
    int32_t c = getNumChannel(config["pfnc_pixelformat"]);
    int32_t framesize = w * h * d * c;

    int32_t num_bitshift = getBitShift(config["pfnc_pixelformat"]);

    std::vector<std::string> bin_files;
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

        int cursor = 0;

        while(cursor < static_cast<int>(filesize)){
            int framecount = *reinterpret_cast<int*>(filecontent + cursor);
            std::cout << framecount << std::endl;

            cv::Mat img(h, w, getOpenCVMatType(d, c));
            std::memcpy(img.ptr(), filecontent + cursor + 4, framesize);
            img = img * pow(2, num_bitshift);
            cv::imshow("First available image component", img);
            cursor += 4 + framesize;

            cv::waitKeyEx(1);
        }
        delete[] filecontent;
    }

    return 0;
}
