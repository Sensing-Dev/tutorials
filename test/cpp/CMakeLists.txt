cmake_minimum_required(VERSION 3.3)
project(SensingDevTutorial)

set(CMAKE_CXX_STANDARD 17)

set(SENSING_DEV_DIR $ENV{SENSING_DEV_ROOT})

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
    target_link_libraries(${TUTORIAL_NAME} PRIVATE ion-core.lib)
    target_link_libraries(${TUTORIAL_NAME} PRIVATE halide.lib)
endfunction()

# Add each tutorial based on the BUILD_TUTORIAL option

add_tutorial_target(fake_camera_display src/fake_camera_display.cpp)
add_tutorial_target(fake_camera_save src/fake_camera_save.cpp)

#
# Allow big object
#
if (MSVC)
    add_definitions(/bigobj)
    message(STATUS "Allow big object")
endif (MSVC)