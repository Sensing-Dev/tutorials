/*

g++ src/load_and_process.cpp -o load_and_process  \
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
#include <iostream>
#include <chrono>

using namespace ion;
std::map<std::string, int32_t> bit_width_map{
  {"BayerBG8", 8}, {"BayerBG10", 16}, {"BayerBG12", 16}};
std::map<std::string, int32_t> num_bit_shift_map{
  {"BayerBG8", 0}, {"BayerBG10", 6}, {"BayerBG12", 4}};
std::map<std::string, std::string> bb_name{
  {"BayerBG8", "image_io_binaryloader_u8x2"},
  {"BayerBG10", "image_io_binaryloader_u16x2"},
  {"BayerBG12", "image_io_binaryloader_u16x2"}};


void pipeline_acquisition_and_process(bool use_cuda, bool display_image, std::string output_directory, std::string prefix, int width, int height, std::string pixelformat, int num_frames){
    std::vector< int > buf_size = std::vector < int >{3, width, height};

    std::vector<Halide::Buffer<uint8_t>> outputs;
    outputs.push_back(Halide::Buffer<uint8_t>(buf_size));

    Buffer<bool> finished(1);

    Builder b;
    if (use_cuda){
      b.set_target(get_host_target().with_feature(Target::CUDA).with_feature(Target::Profile));
    }else{
      b.set_target(get_host_target().with_feature(Target::Profile));
    }
    b.with_bb_module("ion-bb");

    // add binary loader BB to pipeline
    Node n = b.add(bb_name[pixelformat])(&width, &height)
      .set_param(
              Param("output_directory", output_directory),
              Param("prefix", prefix)
      );
    n["finished"].bind(finished);

    if (bit_width_map[pixelformat] == 8){
      n = b.add("base_cast_2d_uint8_to_uint16")(n["output"]);
    }

    n = b.add("image_processing_normalize_raw_image")(n["output"])
      .set_param(
        Param("bit_width", bit_width_map[pixelformat]),
        Param("bit_shift", num_bit_shift_map[pixelformat])
      );

    n  = b.add("image_processing_bayer_demosaic_linear")(n["output"])
        .set_param(
          Param("bayer_pattern", "BGGR"),
          Param("width", width),
          Param("height", height)
          );  //  output(x, y, c)

    n  = b.add("base_denormalize_3d_uint8") (n["output"]);
    n  = b.add("base_reorder_buffer_3d_uint8")(n["output"])
        .set_param(
          Param("dim0", "2"),
          Param("dim1", "0"),
          Param("dim2", "1")
          ); //  output(c, x, y)
    n["output"].bind(outputs[0]);

    int count_run = 1;
    while (true) {
      b.run();
      bool is_finished = finished(0);

      cv::Mat img(height, width, CV_8UC3);
      std::memcpy(img.ptr(), outputs[0].data(), outputs[0].size_in_bytes());
      if (display_image){
        cv::imshow("image" + std::to_string(0), img);
        cv::waitKey(1);
      }

      if (count_run == num_frames || is_finished) {
        break;
      }
      count_run += 1;
    }
    std::cout << "Total number of frames to process: " << count_run << std::endl;
}

int main(int argc, char *argv[])
{
  try{
    // if prefix is imageX if the name of the config is imageX-config.json
    std::string prefix = "image0-";
    // check imageX-config.json
    const int32_t width = 1920;
    const int32_t height = 1080;
    std::string pixelformat = "BayerBG8";

    bool use_cuda = false;
    bool display_image = false;
    std::string output_directory = ".";
    int num_frames = 0;

    if (argc > 1){
      for (int i = 1; i < argc; i++){
          if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--target-cuda") == 0){
            use_cuda = true;
          }else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--display") == 0){
            display_image = true;
          }else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0){
            output_directory = argv[++i];
          }else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--prefix") == 0){
            prefix = argv[++i];
          }else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--num-frames") == 0){
            num_frames = strtol(argv[++i], NULL, 10);
          }else{
            std::cerr << "wrong format of command option." << std::endl;
          }
      }
    }

  pipeline_acquisition_and_process(use_cuda, display_image, output_directory, prefix, width, height, pixelformat, num_frames);

  } catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (const ion::Error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}
