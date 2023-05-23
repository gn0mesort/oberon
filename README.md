# Oberon

Oberon is my exploratory 3D graphics library. Ideally, someday this will be a more or less fully featured library.
Currently, it exists primarily as a demonstration.

## Building

### Linux

Oberon can be built using the included `meson.build` script. To do this, ensure that `meson` is installed as well as
the underlying Ninja build system. Next install the application's dependencies. Currently, Oberon requires the
following libraries:
- libxcb
- libxcb-xkb
- libxcb-input
- libxkbcommon
- libxkbcommon-x11
- libvulkan
- libuuid
- libnng

Additionally, the Vulkan features require [VKFL](https://github.com/gn0mesort/vkfl). VKFL is provided as a submodule
and requires the following dependencies:
- Mako
- defusedxml

Once all of the required dependencies are installed run the following commands within the source directory to build
the software:
```sh
meson setup build
ninja -C build
```

On Ubuntu, specifically, the following script will initialize the source directory so the software can be built:
```sh
# These commands only need to be run once when preparing the source directory.
sudo apt install meson ninja-build python3-venv python3-pip libxcb-dev libxcb-input-dev libxkbcommon-dev libxkbcommon-x11-dev libvulkan-dev
git submodule init
git submodule update
python3 -m venv .venv
# You should be in the virtual environment whenever building.
source .venv/bin/activate
pip3 install -r requirements.txt
# Create the build directory.
meson setup build
# Build the software.
ninja -C build
```

## Configuration

### Linux

Oberon supports the following environment variables
| Environment Variable | Value Type | Description | Example Values |
| -------------------- | ---------- | ----------- | -------------- |
| `OBERON_VK_LAYERS` | A comma separated list of strings. | A list of Vulkan instance layer names that should be loaded during program initialization. | `OBERON_VK_LAYERS=VK_LAYER_KHRONOS_validation,VK_LAYER_MESA_overlay` |
