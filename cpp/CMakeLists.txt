cmake_minimum_required(VERSION 3.3)
project(SensingDevTutorial)

set(CMAKE_CXX_STANDARD 17)

set(SENSING_DEV_DIR $ENV{SENSING_DEV_ROOT})
if(NOT OpenCV_DIR)
    set(OpenCV_DIR $ENV{SENSING_DEV_ROOT}/opencv/build)
endif()

# header files
include_directories(${SENSING_DEV_DIR}/include)
include_directories(${SENSING_DEV_DIR}/include/aravis-0.8)
include_directories(${SENSING_DEV_DIR}/include/glib-2.0)
include_directories(${SENSING_DEV_DIR}/lib/glib-2.0/include)

include_directories(src)

link_directories(${SENSING_DEV_DIR}/bin)
link_directories(${SENSING_DEV_DIR}/lib)

# Define an option for building specific tutorials
set(BUILD_TUTORIAL "all" CACHE STRING "Build specific tutorials; default is all")

# Function to add a tutorial target
function(add_tutorial_target TUTORIAL_NAME SOURCE_FILE)
    add_executable(${TUTORIAL_NAME} ${SOURCE_FILE})
    target_compile_features(${TUTORIAL_NAME} PUBLIC cxx_std_17)
    if (TUTORIAL_NAME STREQUAL "tutorial0_get_device_info" OR TUTORIAL_NAME STREQUAL "tutorial0_set_device_info")
        target_link_libraries(${TUTORIAL_NAME} PRIVATE aravis-0.8.lib)
        target_link_libraries(${TUTORIAL_NAME} PRIVATE gobject-2.0.lib)
    elseif (TUTORIAL_NAME STREQUAL "tutorial4_save_gendc_data" OR TUTORIAL_NAME STREQUAL "tutorial4_save_image_bin_data")
        target_link_libraries(${TUTORIAL_NAME} PRIVATE ion-core.lib)
        target_link_libraries(${TUTORIAL_NAME} PRIVATE halide.lib)
    else()
        find_package(OpenCV REQUIRED PATHS ${OpenCV_DIR})
        target_link_libraries(${TUTORIAL_NAME} PRIVATE ${OpenCV_LIBS})
        target_link_libraries(${TUTORIAL_NAME} PRIVATE ion-core.lib)
        target_link_libraries(${TUTORIAL_NAME} PRIVATE halide.lib)
    endif()
endfunction()

# Add each tutorial based on the BUILD_TUTORIAL option
if(BUILD_TUTORIAL STREQUAL "all")
    add_tutorial_target(tutorial0_get_device_info src/tutorial0_get_device_info.cpp)
    add_tutorial_target(tutorial0_set_device_info src/tutorial0_set_device_info.cpp)
    add_tutorial_target(tutorial1_display src/tutorial1_display.cpp)
    add_tutorial_target(tutorial2_control_camera src/tutorial2_control_camera.cpp)
    add_tutorial_target(tutorial3_getting_frame_count src/tutorial3_getting_frame_count.cpp)
    add_tutorial_target(tutorial4_save_gendc_data src/tutorial4_save_gendc_data.cpp)
    add_tutorial_target(tutorial4_save_image_bin_data src/tutorial4_save_image_bin_data.cpp)
    add_tutorial_target(tutorial5_parse_gendc_data src/tutorial5_parse_gendc_data.cpp)
    add_tutorial_target(tutorial5_parse_image_bin_data src/tutorial5_parse_image_bin_data.cpp)
    message(STATUS "All tutorials are ready to build.")
else()
    foreach(TUTORIAL ${BUILD_TUTORIAL})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${TUTORIAL}.cpp")
            add_tutorial_target(${TUTORIAL} src/${TUTORIAL}.cpp)
            message(STATUS "${TUTORIAL}.cpp is ready to build.")
        else()
            message(WARNING "${TUTORIAL}.cpp does not exist.")
        endif()
    endforeach()
endif()
#
# Allow big object
#
if (MSVC)
    add_definitions(/bigobj)
    message(STATUS "Allow big object")
endif (MSVC)