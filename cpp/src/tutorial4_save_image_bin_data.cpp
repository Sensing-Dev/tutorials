/*

g++ src/tutorial4_save_image_bin_data.cpp -o tutorial4_save_image_bin_data  \
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

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/ioctl.h>
    #include <termios.h>
    #include <fcntl.h>
    #include <unistd.h>

    #define VK_SPACE 0x20

    struct termios SetNonBlockInput(){
        struct termios original_setting, copy_setting;
        tcgetattr (STDIN_FILENO, &original_setting);
        tcgetattr (STDIN_FILENO, &copy_setting);
        copy_setting.c_lflag &= ~ (ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &copy_setting);

        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        return original_setting;
    }

    void ResetTerminal(struct termios terminal_setting){
        tcsetattr(STDIN_FILENO, TCSANOW, &terminal_setting);
    }

    short GetAsyncKeyState(short _)
    {
        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n > 0 && ch == ' '){
            return 1;
        }
        return 0;
    }
#endif

using namespace ion;

std::map<std::string, std::string> acquisition_bb_name{
  {"Mono8", "image_io_u3v_cameraN_u8x2"}, 
  {"Mono10", "image_io_u3v_cameraN_u16x2"}, 
  {"Mono12", "image_io_u3v_cameraN_u16x2"}};

  std::map<std::string, std::string> bin_saver_bb_name{
  {"Mono8", "image_io_binarysaver_u8x2"}, 
  {"Mono10", "image_io_binarysaver_u16x2"}, 
  {"Mono12", "image_io_binarysaver_u16x2"}};

int build_and_process_pipeline(
    std::vector<int32_t>& width, std::vector<int32_t>& height, 
    std::string pixel_format, int num_device, std::string saving_diretctory){
    // pipeline setup
    Builder b;
    b.set_target(ion::get_host_target());
    b.with_bb_module("ion-bb");

    // add BB to pipeline
    // the name of image-acquisition BB would vary depending on its PixelFormat
    Node n = b.add(acquisition_bb_name[pixel_format])()
      .set_params(
        Param("num_devices", num_device),
        Param("frame_sync", true),
        Param("realtime_display_mode", true)
      );

    // create halide buffer for output port
    std::vector<Halide::Buffer<int>> outputs;
    for (int i = 0; i < num_device; ++i){
        outputs.push_back(Halide::Buffer<int>::make_scalar());
    }

    for (int i = 0; i < num_device; ++i){
        int32_t w = width[i];
        int32_t h = height[i];
        std::string prefix = "image" + std::to_string(i) + "-";
        Node child_n = b.add(bin_saver_bb_name[pixel_format])(n["output"][i], n["device_info"][i], n["frame_count"][i], &w, &h)
        .set_params(
            Param("prefix", prefix),
            Param("output_directory", saving_diretctory)
        );
        child_n["output"].bind(outputs[i]);
    }

    int32_t num_run = 0;

    std::cout << "Hit SPACE KEY to stop saving" << std::endl;

    #ifndef _WIN32
        struct termios terminal_setting = SetNonBlockInput();
    #endif

    while(true){
        b.run();
        ++num_run;

        if (GetAsyncKeyState (VK_SPACE) != 0) {
            break;
        }
    }
    std::cout << num_run << " frames are saved under " << saving_diretctory << std::endl;

    #ifndef _WIN32
        ResetTerminal(terminal_setting);
    #endif

    return 0;

}

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

        // The following info can be checked with
        // `arv-tool-0.8 -n "<name of device>" control PixelFormat Width Height`
        std::vector<int32_t> width = {1920, 1920};
        std::vector<int32_t> height = {1080, 1080};
        std::string pixel_format = "Mono12";

        int32_t num_device = 2;

        int ret = build_and_process_pipeline(width, height, pixel_format, num_device, ss.str());

    } catch(std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const ion::Error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}