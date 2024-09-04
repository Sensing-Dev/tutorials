/*

g++ src/tutorial4_save_gendc_data.cpp -o tutorial4_save_gendc_data  \
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

int build_and_process_pipeline(std::vector<int32_t>& payloadsize, int num_device, std::string saving_diretctory){
    // pipeline setup
    Builder b;
    b.set_target(ion::get_host_target());
    b.with_bb_module("ion-bb");

    // add BB to pipeline
    Node n = b.add("image_io_u3v_gendc")()
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

    if (num_device == 2){
        int32_t payloadsize1 = payloadsize[1];
        Node n1 = b.add("image_io_binary_gendc_saver")(n["gendc"][1], n["device_info"][1], &payloadsize1)
        .set_params(
            Param("prefix", "gendc1-"),
            Param("output_directory", saving_diretctory)
        );
        n1["output"].bind(outputs[1]);
    }
    int32_t payloadsize0 = payloadsize[0];
    n = b.add("image_io_binary_gendc_saver")(n["gendc"][0], n["device_info"][0], &payloadsize0)
        .set_params(
            Param("prefix", "gendc0-"),
            Param("output_directory", saving_diretctory)
        );

    // bind halide buffer for output port
    n["output"].bind(outputs[0]);

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
        // `arv-tool-0.8 -n "<name of device>" control PayloadSize`
        std::vector<int32_t> payloadsize = {2074880, 2074880};

        int32_t num_device = 2;

        int ret = build_and_process_pipeline(payloadsize, num_device, ss.str());

    } catch(std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const ion::Error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}