# libkipr
Library for interfacing with KIPR Robot Controllers.

Documentation can be viewed at https://www.kipr.org/doc/index.html or by clicking the "Help" button in the KIPR Software Suite IDE.

# CMake Options

Each of the following options may be specified when executing CMake by prefixing the option with `-D` (e.g., `-Dwith_accel=OFF`).

## Modules
  - `with_accel` (default: `ON`) - Build accelerometer support.
  - `with_analog` (default: `ON`) - Build analog sensor support.
  - `with_audio` (default: `ON`) - Build audio support.
  - `with_battery` (default: `ON`) - Build battery support.
  - `with_botball` (default: `ON`) - Build botball support.
  - `with_camera` (default: `ON`) - Build camera support.
  - `with_compass` (default: `ON`) - Build compass support.
  - `with_console` (default: `ON`) - Build console support.
  - `with_create` (default: `ON`) - Build iRobot Create 2 support.
  - `with_digital` (default: `ON`) - Build digital sensor support.
  - `with_graphics` (default: `ON`) - Build graphics support (requires X11 development files, such as `x11proto-dev` on Debian/Ubuntu).
  - `with_gyro` (default: `ON`) - Build gyroscope support.
  - `with_magneto` (default: `ON`) - Build magnetometer support.
  - `with_motor` (default: `ON`) - Build motor support.
  - `with_network` (default: `ON`) - Build network support.
  - `with_servo` (default: `ON`) - Build servo support.
  - `with_tello` (default: `ON`) - Build Tello support.
  - `with_thread` (default: `ON`) - Build thread support.
  - `with_time` (default: `ON`) - Build time support.
  - `with_wait_for` (default: `ON`) - Build wait_for support.

## Bindings
  - `with_python_binding` (default: `ON`) - Build Python binding (requires Python 3+ development files, such as `libpython3.10-dev` on Debian/Ubuntu).

## Miscellaneous
  - `with_documentation` (default: `ON`) - Build documentation support (requires `doxygen` installed on system).
  - `with_tests` (default: `ON`) - Build tests.

## Dummy Build
  - `DUMMY` (default: `OFF`) - Build a dummy build for use on computer

# Cross-compiling to aarch64-linux-gnu (e.g., Wombat)

```bash
apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
cd libkipr
cmake -Bbuild -DCMAKE_TOOLCHAIN_FILE=$(pwd)/toolchain/arm-linux-gnueabihf.cmake .
```

# Cross-compiling to JavaScript/WASM (e.g., Simulator)

## Without Python Support
libkipr can be compiled to statically link to a C program that has been compiled to emscripten. The resulting `libkipr.a` should be used.
```
source emsdk/emsdk_env.sh
cd libkipr
emcmake cmake -Bbuild -Dwith_graphics=OFF -Dwith_camera=OFF -Dwith_tello=OFF -Dwith_python_binding=OFF .
```

## With Python Support
libkipr can be compiled to dynamically link to a `cpython` that has been compiled to emscripten. The resulting `kipr.wasm` should be placed in `cpython`'s virtual filesystem on the `PYTHONPATH` (see simulator for details).
```
source emsdk/emsdk_env.sh
cd libkipr
emcmake cmake -Bbuild -Dwith_graphics=OFF -Dwith_camera=OFF -Dwith_tello=OFF -DPYTHON_LIBRARY=$PYTHON_LIBRARY -DPYTHON_INCLUDE_DIR=$PYTHON_INCLUDE_DIR -Dwasm=ON .
```
where:
  - `$PYTHON_LIBRARY` is the `libpythonVERSION.a` file that has been compiled to emscripten-browser.
  - `$PYTHON_INCLUDE_DIR` is that python's include directory.

# AI Camera Testing

You can install the deb file hosted [here](https://files.kipr.org/kipr-1.2.1-Linux.deb), which should include all changes up to 2025-10-10.
If you want to test newer changes, you can compile using the instructions above or using the new Docker build.

Make sure docker is set up properly for your user.

```bash
./build.sh
```

Install the produced `deb` normally and reboot, then add the `./ai-camera-link.nmconnection` to your wombat, which will automatically connect when the AI camera is plugged in.

```bash
scp ai-camera-link.nmconnection kipr@<wombat_ip>:~
ssh kipr@<wombat_ip> 'sudo bash -c "mv /home/kipr/ai-camera-link.nmconnection /etc/NetworkManager/system-connections/ && chown root:root /etc/NetworkManager/system-connections/ai-camera-link.nmconnection && sudo reboot"'
```

## Notes on AI Camera beta

This patch should only be used for testing as it will cause only the AI camera to be supported.
This restriction will be lifted in an upcoming release.
Important parts not included in this release:

1. AI camera auto-detect. Important because the AI camera uses IP over USB as its connection. Auto-detect will look for the camera over the USB IP network.
2. Model download. Download AI models from the Wombat.

The object data structures found by the AI camera are transferred into a configured blob tracking channel, which can be done through BOTUI->settings->channels.
The added channel is currently configured using "QR code scanner."
A future release will use a more appropriate name.

The data fields of the channel object structure are used as follows:

- `m_centroid` - centroid
- `m_bounding_box` - bounding box
- `m_confidence` - confidence
- `m_data` - string from model that indicates object type
- `m_length` - length of string

In short, the same as the blob tracking object and with the added bonus of being accessed by existing functions.

# License

libkipr is licensed under the terms of the [GPLv3 License](LICENSE).

# Contributing

Want to Contribute? Start Here!:
https://github.com/kipr/KIPR-Development-Toolkit
