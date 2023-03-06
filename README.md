# Oberon

## Building

### Linux

Oberon can be built using the included `meson.build` script. To do this, ensure that `meson` is installed as well as
the underlying Ninja build system. Next install the application's dependencies. Currently, Oberon requires the
following libraries:
- libxcb
- libxcb-input
- libX11
- libX11-xcb
- libxkbcommon
- libxkbcommon-x11
- libvulkan

Additionally, the Vulkan features require [vkfl](https://github.com/gn0mesort/vkfl). vkfl is provided as a submodule
and requires the following dependencies:
- Mako
- defusedml

Once all of the required dependencies are installed run the following commands within the source directory to build
the software:
```sh
meson setup build
ninja -C build
```

On Ubuntu, specifically, the following script will initialize the source directory so the software can be built:
```sh
# These commands only needs to be run once when preparing the source directory.
sudo apt install meson ninja-build python3-venv python3-pip libxcb-dev libxcb-input-dev libx11-dev libx11-xcb-dev libxkbcommon-dev libxkbcommon-x11-dev libvulkan-dev
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
| Environment Variable | Value Type                      | Description                                                                                | Example Values                                                       |
| -------------------- | ------------------------------- | ------------------------------------------------------------------------------------------ | -------------------------------------------------------------------- |
| `OBERON_VK_LAYERS`   | Comma separated list of strings | A list of Vulkan instance layer names that should be loaded during program initialization. | `OBERON_VK_LAYERS=VK_LAYER_KHRONOS_validation` or                    |
|                      |                                 | If the application is built in debug mode (i.e., `NDEBUG` is not defined) and this         | `OBERON_VK_LAYERS=VK_LAYER_KHRONOS_validation,VK_LAYER_MESA_overlay` |
|                      |                                 | variable is set then the value *MUST* include a layer providing `VK_EXT_debug_utils` and   |                                                                      |
|                      |                                 | `VK_EXT_validation_features` (if they are not supported by the instance directly).         |                                                                      |
