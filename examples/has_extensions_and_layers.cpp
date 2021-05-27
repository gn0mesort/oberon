#include <cstdio>

#include <vulkan/vulkan.h>

#include <oberon/memory.hpp>
#include <oberon/context.hpp>

oberon::cstring to_cstring(const bool b) {
  return b ? "true" : "false";
}

int main() {

  std::printf(
    "Has extension \"" VK_KHR_SURFACE_EXTENSION_NAME "\": %s\n",
    to_cstring(oberon::context::has_extension(VK_KHR_SURFACE_EXTENSION_NAME))
  );
  std::printf(
    "Has layer \"VK_LAYER_KHRONOS_validation\": %s\n",
    to_cstring(oberon::context::has_layer("VK_LAYER_KHRONOS_validation"))
  );
  std::printf(
    "\"VK_LAYER_KHRONOS_validation\" has extension \"" VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME "\": %s\n",
    to_cstring(
      oberon::context::layer_has_extension("VK_LAYER_KHRONOS_validation", VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME)
    )
  );
}
