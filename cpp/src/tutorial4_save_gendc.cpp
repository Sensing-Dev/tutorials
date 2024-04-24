/*

g++ src/tutorial4_save_gendc.cpp -o tutorial4_save_gendc  \
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
#include "gendc_separator/ContainerHeader.h"
#include "gendc_separator/tools.h"

#ifdef _WIN32
    #include <conio.h>
#else
    #include <sys/ioctl.h>
    #include <termios.h>

    bool _kbhit()
    {
        termios term;
        tcgetattr(0, &term);

        termios term_cpy = term;
        term_cpy.c_lflag &= ~ICANON;
        tcsetattr(0, TCSANOW, &term_cpy);

        int byteswaiting;
        ioctl(0, FIONREAD, &byteswaiting);

        tcsetattr(0, TCSANOW, &term);

        return byteswaiting > 0;
    }
#endif

using namespace ion;




int build_and_process_pipeline(int width, int height, std::string pixel_format, int payloadsize, int num_device, std::string saving_diretctory){
    // pipeline setup
    Builder b;
    b.set_target(ion::get_host_target());
    b.with_bb_module("ion-bb");

    // add BB to pipeline
    Node n = b.add("image_io_u3v_gendc")()
      .set_param(
        Param("num_devices", num_device),
        Param("frame_sync", true),
        Param("realtime_diaplay_mode", false)
      );

    n = b.add("image_io_binary_gendc_saver")(n["gendc"], n["device_info"], &payloadsize)
        .set_param(
            Param("num_devices", num_device),
            Param("output_directory", saving_diretctory),
            Param("input_gendc.size", num_device),
            Param("input_deviceinfo.size", num_device)
        );

    // create halide buffer for output port
    Halide::Buffer<int> output = Halide::Buffer<int>::make_scalar();
    n["output"].bind(output);

    int32_t num_run = 0;

    std::cout << "Hit any key to stop saving" << std::endl;

    while(true){
        b.run();
        ++num_run;

        if (_kbhit()) {
            break;
        }
    }
    std::cout << num_run << " frames are saved under " << saving_diretctory << std::endl;

    return 0;

}

int main(int argc, char* argv[]){
    
    try{
        // Create a saving directory
        std::string saving_directory_prefix = "tutorial_save_gendc_";
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

        // The following info can be checked with
        // `arv-tool-0.8 -n "<name of device>" control PixelFormat Width Height PayloadSize`
        int32_t width = 1920;
        int32_t height = 1080;
        std::string pixelformat = "Mono8";
        int32_t payloadsize = 2074880;

        int32_t num_device = 2;

        int ret = build_and_process_pipeline(width, height, pixelformat, payloadsize, num_device, ss.str());

    } catch(std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const ion::Error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}