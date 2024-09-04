/*

g++ src/tutorial2_control_camera.cpp -o tutorial2_control_camera  \
-I /opt/sensing-dev/include -I /opt/sensing-dev/include/aravis-0.8 \
-I /opt/sensing-dev/include/opencv4 \
-L /opt/sensing-dev/lib \
-L /opt/sensing-dev/lib/x86_64-linux-gnu \
-lHalide -lion-core -ldl -lpthread -lopencv_core \
-lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc \
-laravis-0.8 -lgobject-2.0 \
`pkg-config --cflags --libs glib-2.0`

*/

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <ion/ion.h>

#include <exception>
#include <string>
#include <map>

using namespace ion;

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

double gain0 = 35.0;
double exposuretime0 = 50.0;
double gain1 = 47.0;
double exposuretime1 = 100.0;

template<typename T>
int video(int width, int height, std::string pixel_format, int num_device){
    // pipeline setup
    Builder b;
    b.set_target(ion::get_host_target());
    b.with_bb_module("ion-bb");

    // add BB to pipeline
    Node n;
    if (num_device == 1){
      n = b.add(bb_name[pixel_format])(&gain0, &exposuretime0)
        .set_params(
          Param("num_devices", num_device),
          Param("frame_sync", true),
          Param("realtime_display_mode", true),
          Param("enable_control", true),
          Param("gain_key", "Gain"),
          Param("exposure_key", "ExposureTime")
        );
    }else if (num_device == 2){
      n = b.add(bb_name[pixel_format])(&gain0, &exposuretime0, &gain1, &exposuretime1)
        .set_params(
          Param("num_devices", num_device),
          Param("frame_sync", true),
          Param("realtime_display_mode", true),
          Param("enable_control", true),
          Param("gain_key", "Gain"),
          Param("exposure_key", "ExposureTime")
        );
    }else{
      throw("Set the same number of gains and exposure times as the number of devices.");
    }
    
    // portmapping from output port to buffer
    std::vector< int > buf_size = std::vector < int >{ width, height };
    if (pixel_format == "RGB8"){
        buf_size.push_back(3);
    }
    std::vector<Halide::Buffer<T>> output;
    for (int i = 0; i < num_device; ++i){
      output.push_back(Halide::Buffer<T>(buf_size));
    }
    n["output"].bind(output);

    int coef =  positive_pow(2, num_bit_shift_map[pixel_format]);
    int user_input = -1;

    while(user_input == -1)
    {
      // JIT compilation and execution of pipelines with Builder.
      b.run();
      
      // Convert the retrieved buffer object to OpenCV buffer format.
      for (int i = 0; i < num_device; ++i){
        cv::Mat img(height, width, opencv_mat_type[pixel_format], output[i].data());
        img *= coef;
        cv::imshow("image" + std::to_string(i), img);
      }

      // Wait for 1ms
      user_input = cv::waitKeyEx(1);
    }
    return 0;
}

int main(int argc, char *argv[])
{
  try{
    int32_t width = 1920;
    int32_t height = 1080;
    int32_t num_device = 1;
    std::string pixelformat = "Mono8";

    if (pixelformat == "Mono8"){
      int ret = video<uint8_t>(width, height, pixelformat, num_device);
    }else{
      int ret = video<uint16_t>(width, height, pixelformat, num_device);
    }

  } catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (const ion::Error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}