/*

g++ src/tutorial0_get_device_info.cpp -o tutorial0_get_device_info  \
-I /opt/sensing-dev/include -I /opt/sensing-dev/include/aravis-0.8 \
-L /opt/sensing-dev/lib \
-L /opt/sensing-dev/lib/x86_64-linux-gnu \
-ldl -lpthread \
-laravis-0.8 -lgobject-2.0 \
`pkg-config --cflags --libs glib-2.0`

*/

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
        printf("%20s : %" G_GUINT64_FORMAT "\n",
            "Width",
            arv_device_get_integer_feature_value(device, "Width", &error));
                if (error){
            throw std::runtime_error(error->message);
        }
        printf("%20s : %" G_GUINT64_FORMAT "\n",
            "Height",
            arv_device_get_integer_feature_value(device, "Height", &error));
                if (error){
            throw std::runtime_error(error->message);
        }
        printf("%20s : %" G_GUINT64_FORMAT "\n",
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

        g_object_unref (device);
    }

    return 0;
}