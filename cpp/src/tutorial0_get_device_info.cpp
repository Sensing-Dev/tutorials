#include <exception>
#include <iostream>
#include "arv.h"

int main(int argc, char *argv[])
{
    arv_update_device_list ();
    unsigned int n_devices = arv_get_n_devices ();
    if (n_devices < 1){
      throw std::runtime_error("Device is not connected.");
    }

    GError *error = nullptr;

    for (unsigned int i = 0; i < n_devices; ++i){
        const char* dev_id = arv_get_device_id (i);
        ArvDevice* device = arv_open_device(dev_id, nullptr);

        printf("=== device {%d} information ===========================\n", i);

        printf("%20s : %s\n",
            "Device Model Name",
            arv_device_get_string_feature_value(device, "DeviceModelName", &error));
        if (error){
            throw std::runtime_error(error->message);
        }
        printf("%20s : %li\n",
            "Width",
            arv_device_get_integer_feature_value(device, "Width", &error));
                if (error){
            throw std::runtime_error(error->message);
        }
        printf("%20s : %li\n",
            "Height",
            arv_device_get_integer_feature_value(device, "Height", &error));
                if (error){
            throw std::runtime_error(error->message);
        }
        printf("%20s : %li\n",
            "PayloadSize",
            arv_device_get_integer_feature_value(device, "PayloadSize", &error));
                if (error){
            throw std::runtime_error(error->message);
        }
        printf("%20s : %s\n",
            "PixelFormat",
            arv_device_get_string_feature_value(device, "PixelFormat", &error));
        if (error){
            throw std::runtime_error(error->message);
        }
    }

    return 0;
}