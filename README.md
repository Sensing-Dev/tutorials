# tutorials

See the details in 
https://sensing-dev.github.io/doc/tutorials/intro/index.html

## Windows(C++)

You can use CMake to build each tutorials.

```bash
cd cpp
mkdir build && cd build
cmake ../
cmake --build . --config Release
```

If you did not OpenCV with Sensing-Dev installer script, you need to set `-DOpenCV_DIR` to specify `<where you installed opencv>/build` as follows:

```bash
cmake -DOpenCV_DIR=<where you installed opencv>/build ../
cmake --build . --config Release
```

As a default, it build all tutorials. To build a specific tutorial, add the option of `-DBUILD_TUTORIAL`; for example,

```bash
cmake -DOpenCV_DIR=<where you installed opencv>/build -DBUILD_TUTORIAL=tutorial0_get_device_info ../
cmake --build . --config Release
```

## Linux(C++)

Linux version is supported v24.05.00 or later.