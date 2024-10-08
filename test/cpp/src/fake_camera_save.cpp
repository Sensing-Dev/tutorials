/*

g++ src/fake_camera_save.cpp -o fake_camera_save  \
-I /opt/sensing-dev/include \
-L /opt/sensing-dev/lib \
-L /opt/sensing-dev/lib/x86_64-linux-gnu \
-lHalide -lion-core -ldl -lpthread

*/
#include <filesystem>
#include <iostream>
#include <exception>
#include <ctime>
#include <sstream>

#include <ion/ion.h>


using namespace ion;

std::map<std::string, std::string> acquisition_bb_name{
  {"Mono8", "image_io_u3v_cameraN_u8x2"},
  {"Mono16", "image_io_u3v_cameraN_u16x2"}};

  std::map<std::string, std::string> bin_saver_bb_name{
  {"Mono8", "image_io_binarysaver_u8x2"},
  {"Mono16", "image_io_binarysaver_u16x2"}};


int main(int argc, char* argv[]){

    try{
        // Create a saving directory
        std::string saving_directory_prefix = "tutorial_save_image_bin_";
        std::time_t now;
        std::time(&now);
        std::stringstream ss;

        #ifdef _MSC_VER
        std::tm tm;
        localtime_s(&tm, &now);
        ss << saving_directory_prefix << std::put_time(&tm, "%Y%m%d%H%M%S");
        #else
        std::tm* tm = std::localtime(&now);
        ss << saving_directory_prefix << std::put_time(tm, "%Y%m%d%H%M%S");
        #endif

        std::filesystem::create_directory(ss.str());

        std::string saving_directory = ss.str();

        // The following info can be checked with
        // `arv-tool-0.8 -n "<name of device>" control PixelFormat Width Height`
        int width = 128;
        int height = 128;
        std::string pixel_format = "Mono8";

        int32_t num_device = 2;

        // pipeline setup
        Builder b;
        b.set_target(ion::get_host_target());
        b.with_bb_module("ion-bb");

        // add BB to pipeline
        // the name of image-acquisition BB would vary depending on its PixelFormat
        Node n = b.add(acquisition_bb_name[pixel_format])()
          .set_params(
            Param("num_devices", num_device),
            Param("force_sim_mode", true),
            Param("width", width),
            Param("height", height)
          );

        // create halide buffer for output port
        std::vector<Halide::Buffer<int>> outputs;
        for (int i = 0; i < num_device; ++i){
            outputs.push_back(Halide::Buffer<int>::make_scalar());
        }

        for (int i = 0; i < num_device; ++i){
            std::string prefix = "image" + std::to_string(i) + "-";
            Node child_n = b.add(bin_saver_bb_name[pixel_format])(n["output"][i], n["device_info"][i], n["frame_count"][i], &width, &height)
            .set_params(
                Param("prefix", prefix),
                Param("output_directory", saving_directory)
            );
            child_n["output"].bind(outputs[i]);
        }

        int32_t num_run = 0;

       for (int i = 0;i<100;i++){
            b.run();
            ++num_run;

        }
        std::cout << num_run << " frames are saved under " << saving_directory << std::endl;

    } catch(std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const ion::Error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}