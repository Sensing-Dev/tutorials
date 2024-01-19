import os
os.add_dll_directory(os.path.join(os.environ["SENSING_DEV_ROOT"], "bin"))

import gi
gi.require_version("Aravis", "0.8")
from gi.repository import Aravis

gain_key = "Gain"
exposuretime_key = "ExposureTime"

if __name__ == "__main__":
    
    # Aravis list up the connected devices
    Aravis.update_device_list()

    num_device = Aravis.get_n_devices()
    if  num_device == 0:
        raise Exception("No device was found.")

    for i in range(num_device):
    # Access to the ith device
        device = Aravis.Camera.new(Aravis.get_device_id(i)).get_device()
        # Get GenICam feature values
        devicemodelname = device.get_string_feature_value("DeviceModelName")
        current_gain = device.get_float_feature_value(gain_key)
        current_exposuretime = device.get_float_feature_value(exposuretime_key)

        print("=== device {} information ===========================".format(i))
        print("{0:20s} : {1}".format("gain_key", current_gain))
        print("{0:20s} : {1}".format("exposuretime_key", current_exposuretime))

        print("=>")

        # Get GenICam feature values
        new_gain = current_gain + 10.0
        new_exposuretime = current_exposuretime + 20.0
        device.set_float_feature_value(gain_key, new_gain)
        device.set_float_feature_value(exposuretime_key, new_exposuretime)

        # Get GenICam feature values again
        current_gain = device.get_float_feature_value(gain_key)
        current_exposuretime = device.get_float_feature_value(exposuretime_key)

        print("{0:20s} : {1}".format("gain_key", current_gain))
        print("{0:20s} : {1}".format("exposuretime_key", current_exposuretime))

        # Release the device and close the device
        del device

    Aravis.shutdown()

