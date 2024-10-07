/*

g++ src/fake_camera_display.cpp -o fake_camera_display  \
-I /opt/sensing-dev/include -I /opt/sensing-dev/include/aravis-0.8 \
-L /opt/sensing-dev/lib \
-L /opt/sensing-dev/lib/x86_64-linux-gnu \
-lHalide -lion-core -ldl -lpthread \
-laravis-0.8 -lgobject-2.0 \
`pkg-config --cflags --libs glib-2.0`

*/

#include <ion/ion.h>

#include <exception>
#include <string>
#include <map>

using namespace ion;

std::map<std::string, std::string> bb_name{
  {"Mono8", "image_io_u3v_cameraN_u8x2"},
  {"Mono10", "image_io_u3v_cameraN_u16x2"},
  {"Mono12", "image_io_u3v_cameraN_u16x2"}};

int main(int argc, char *argv[])
{
  try{
    int32_t width = 640;
    int32_t height = 480;
    int32_t num_device = 2;
    std::string pixelformat = "Mono8";


    Builder b;
    b.set_target(ion::get_host_target());
    b.with_bb_module("ion-bb");

    // add BB to pipeline
    Node n = b.add(bb_name[pixelformat])()
      .set_params(
        Param("num_devices", num_device),
        Param("force_sim_mode", true),
        Param("width", width),
        Param("height", height)
      );

    // portmapping from output port to output buffer
    std::vector< int > buf_size = std::vector < int >{ width, height };

    std::vector<Halide::Buffer<uint8_t>> output;
    for (int i = 0; i < num_device; ++i){
      output.push_back(Halide::Buffer<uint8_t>(buf_size));
    }
    n["output"].bind(output);

    for (int i = 0;i<5;i++)
    {
      // JIT compilation and execution of pipelines with Builder.
      b.run();
      // Convert the retrieved buffer object to OpenCV buffer format.
      for (int i = 0; i < num_device; ++i){
        uint8_t img[height][width];
        std::memcpy(img, output[i].data(), output[i].size_in_bytes());

        for(int i = 0; i < height; i++)
        {
          for(int j = 0; j < width; j++){
               std::cout << unsigned(img[i][j]) << " ";
          }
          std::cout << std::endl;
        }
      }
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