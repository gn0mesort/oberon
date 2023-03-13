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
| Environment Variable | Value Type | Description | Example Values |
| -------------------- | ---------- | ----------- | -------------- |
| `OBERON_VK_LAYERS` | A comma separated list of strings. | A list of Vulkan instance layer names that should be loaded during program initialization. | `OBERON_VK_LAYERS=VK_LAYER_KHRONOS_validation,VK_LAYER_MESA_overlay` |
| `XDG_CONFIG_HOME` | A valid filesystem path. | Used to define the application configuration directory. See [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/index.html). | See [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/index.html). |
| `XDG_CACHE_HOME` | A valid filesystem path. | Used to define the application cache directory. See [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/index.html). | See [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/index.html). |
| `XDG_DATA_HOME` | A valid filesystem path. | Used to define the application mutable data directory. See [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/index.html). | See [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/index.html). |
| `XDG_DATA_DIRS` | A colon separated list of filesystem paths. | Used to define search paths when searching for immutable data files. See [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/index.html). | See [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/index.html). |
| `XDG_CONFIG_DIRS` | A colon separated list of filesystem paths. | Used to define search paths when searching for configuration files. See [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/index.html). | See [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/index.html). |
| `HOME` | A valid filesystem path. | Used to the define the application home directory. | `HOME=/home/user` |
