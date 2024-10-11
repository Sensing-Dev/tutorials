import os
os.add_dll_directory(os.path.join(os.environ["SENSING_DEV_ROOT"], "bin"))

import gi
gi.require_version("Aravis", "0.8")
from gi.repository import Aravis

if __name__ == "__main__":
    
    # Aravis list up the connected devices
    Aravis.update_device_list()

    num_device = Aravis.get_n_devices()
    if  num_device == 0:
        raise Exception("No device was found.")

    for i in range(num_device):
    # Access to the ith device
        deviceid = Aravis.get_device_id(i)
        device = Aravis.Camera.new(deviceid).get_device()
        # Get GenICam feature values
        devicemodelname = device.get_string_feature_value("DeviceModelName")
        width = device.get_integer_feature_value("Width")
        height = device.get_integer_feature_value("Height")
        payloadsize = device.get_integer_feature_value("PayloadSize")
        pixelformat = device.get_string_feature_value("PixelFormat")

        # Release the device and close the device
        del device

        # Display the obtained device information
        print("=== device {} information ===========================".format(i))
        print("{0:20s} : {1}".format("Device ID", deviceid))
        print("{0:20s} : {1}".format("Device Model Name", devicemodelname))
        print("{0:20s} : {1}".format("Wdith", width))
        print("{0:20s} : {1}".format("Height", height))
        print("{0:20s} : {1}".format("PayloadSize", payloadsize))
        print("{0:20s} : {1}".format("PixelFormat", pixelformat))
        
    Aravis.shutdown()

