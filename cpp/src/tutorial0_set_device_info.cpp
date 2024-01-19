#include <exception>
#include <iostream>
#include "arv.h"

#define GAIN_KEY "Gain"
#define EXPOSURETIME_KEY "ExposureTime"

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

        // Get and print the current device info

        printf("%20s : %s\n",
            "Device Model Name",
            arv_device_get_string_feature_value(device, "DeviceModelName", &error));
        if (error){
            throw std::runtime_error(error->message);
        }

        double current_gain = arv_device_get_float_feature_value(device, GAIN_KEY, &error);
        if (error){
            throw std::runtime_error(error->message);
        }
        printf("%20s : %lf\n", GAIN_KEY, current_gain);
        
        double current_exposuretime = arv_device_get_float_feature_value(device, EXPOSURETIME_KEY, &error);
        if (error){
            throw std::runtime_error(error->message);
        }
        printf("%20s : %lf\n", EXPOSURETIME_KEY, current_exposuretime);
            
        printf("=>\n");

        // Set the new values of Gain and ExposureTime to the device
        double new_gain = current_gain + 10.0;
        arv_device_set_float_feature_value(device, GAIN_KEY, new_gain, &error);
        if (error){
            throw std::runtime_error(error->message);
        }

        double new_exposuretime = current_exposuretime + 20.0;
        arv_device_set_float_feature_value(device, EXPOSURETIME_KEY, new_exposuretime, &error);
        if (error){
            throw std::runtime_error(error->message);
        }

        // Get and print the current device info again
        current_gain = arv_device_get_float_feature_value(device, GAIN_KEY, &error);
        if (error){
            throw std::runtime_error(error->message);
        }
        printf("%20s : %lf\n", GAIN_KEY, current_gain);
        
        current_exposuretime = arv_device_get_float_feature_value(device, EXPOSURETIME_KEY, &error);
        if (error){
            throw std::runtime_error(error->message);
        }
        printf("%20s : %lf\n", EXPOSURETIME_KEY, current_exposuretime);
     

        g_object_unref (device);
    }

    return 0;
}