#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <ion/ion.h>

#include <exception>
#include <string>
#include <map>

using namespace ion;

#ifdef _WIN32
    #define MODULE_NAME "ion-bb"
#else
    #define MODULE_NAME "libion-bb.so"
#endif


#define FEATURE_GAIN_KEY "Gain"
#define FEATURE_EXPOSURE_KEY "ExposureTime"


std::map<std::string, int32_t> num_bit_shift_map{
  {"Mono8", 0}, {"Mono10", 6}, {"Mono12", 4}};
std::map<std::string, int32_t> opencv_mat_type{
  {"Mono8", CV_8UC1}, {"Mono10", CV_16UC1}, {"Mono12", CV_16UC1}};
std::map<std::string, std::string> bb_name{
  {"Mono8", "image_io_u3v_cameraN_u8x2"}, 
  {"Mono10", "image_io_u3v_cameraN_u16x2"}, 
  {"Mono12", "image_io_u3v_cameraN_u16x2"}};

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


template<typename T>
int video(int width, int height, std::string pixel_format, int num_device){
    // pipeline setup
    Builder b;
    b.set_target(Halide::get_host_target());
    b.with_bb_module(MODULE_NAME);

    // set port
    Port dispose_p{ "dispose",  Halide::type_of<bool>() };
    Port gain_p{ "gain", Halide::type_of<double>(), 1 };
    Port exposure_p{ "exposure", Halide::type_of<double>(), 1 };

    Node n = b.add(bb_name[pixel_format])(dispose_p, gain_p, exposure_p)
      .set_param(
        Param{"num_devices", std::to_string(num_device)},
        Param{"pixel_format_ptr", pixel_format},
        Param{"frame_sync", "true"},
        Param{"gain_key", FEATURE_GAIN_KEY},
        Param{"exposure_key", FEATURE_EXPOSURE_KEY},
        Param{"realtime_diaplay_mode", "true"}
      );

    double *gains = (double*) malloc (sizeof (double) * num_device);
    double *exposures = (double*) malloc (sizeof (double) * num_device);
    for (int i = 0; i < num_device; ++i){
        gains[i] = 40.0;
        exposures[i] = 100.0;
    }


    Halide::Buffer<double> gain_buf(gains, std::vector< int >{num_device});
    Halide::Buffer<double> exposure_buf(exposures, std::vector< int >{num_device});


    PortMap pm;
    pm.set(gain_p, gain_buf);
    pm.set(exposure_p, exposure_buf);

    std::vector< int > buf_size = std::vector < int >{ width, height };
    if (pixel_format == "RGB8"){
        buf_size.push_back(3);
    }
    std::vector<Halide::Buffer<T>> output;
    for (int i = 0; i < num_device; ++i){
      output.push_back(Halide::Buffer<T>(buf_size));
    }
    pm.set(n["output"], output);
    pm.set(dispose_p, false);
    int user_input = -1;
    while(true)
    {
     
      // JIT compilation and execution of pipelines with Builder.
      try {
          pm.set(dispose_p, user_input != -1);
          b.run(pm);
      }catch(std::exception& e){
          // e.what() shows the error message if pipeline build/run was failed.
          std::cerr << "Failed to build pipeline" << std::endl;
          std::cerr << e.what() << std::endl;
          exit(1);
      }
      if (user_input!=-1){
         break;
      }
      // Convert the retrieved buffer object to OpenCV buffer format.
      for (int i = 0; i < num_device; ++i){
        cv::Mat img(height, width, opencv_mat_type[pixel_format]);
        std::memcpy(img.ptr(), output[i].data(), output[i].size_in_bytes());
        img *= positive_pow(2, num_bit_shift_map[pixel_format]);
        cv::imshow("image" + std::to_string(i), img);
      }
   
      // Wait for key input
      //   When any key is pressed, close the currently displayed image and proceed to the next frame.
      user_input = cv::waitKeyEx(1);
     
    }

    return 0;
}

int main(int argc, char *argv[])
{
  try{
     // replace following values using real camera info
    int32_t width = 1920;
    int32_t height = 1080;
    int32_t num_device = 2;
    std::string pixelformat = "Mono8";

    if (pixelformat == "Mono8"){
      int ret = video<uint8_t>(width, height, pixelformat, num_device);
    }else{
      int ret = video<uint16_t>(width, height, pixelformat, num_device);
    }


  }catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    exit(1);
  }
  return 0;
}