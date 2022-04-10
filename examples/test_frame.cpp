#include <cstdio>

#include <unistd.h>

#include <oberon/linux/system.hpp>

int oberon_main(oberon::system& sys, oberon::onscreen_context& ctx, oberon::render_window& win) {
  return 0;
}

int main() {
  auto sys = oberon::linux::onscreen_system{ };
#ifndef NDEBUG
  using namespace oberon::linux;
  auto layers = std::array<oberon::cstring, 1>{ "VK_LAYER_KHRONOS_validation" };
  sys.set_parameter(SYS_PARAM_VULKAN_REQUIRED_LAYERS, reinterpret_cast<oberon::uptr>(std::data(layers)));
  sys.set_parameter(SYS_PARAM_VULKAN_REQUIRED_LAYER_COUNT, std::size(layers));
  sys.set_parameter(SYS_PARAM_VULKAN_DEBUG_MESSENGER_ENABLE, true);
#endif
  return sys.run(oberon_main);
}
