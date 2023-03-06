#ifndef OBERON_LINUX_VK_HPP
#define OBERON_LINUX_VK_HPP

#include <X11/Xlib-xcb.h>

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_xcb.h>
#undef VK_NO_PROTOTYPES

#include <vkfl.hpp>

#include "../errors.hpp"

/// @cond
// Since prototypes are disabled in vulkan_core.h this is required to provide the basic loader function to vkfl.
// Without this, all vkfl::loaders would need to be allocated dynamically and libvulkan would need to be resolved
// via dlopen/dlsym. I think relying on the dynamic linker is less of a pain.
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance, const char*);
/// @endcond

/**
 * @def OBERON_LINUX_VK_DECLARE_PFN
 * @brief Declare a Vulkan function pointer and resolve its value using vkfl.
 * @param loader A vkfl::loader that is complete to the required stage. That means, for example, to declare a device
 *               function the vkfl::loader must return non-null for vkfl::loader::loaded_device.
 * @param function The name of the function to declare. This is the function name as defined by the Vulkan
 *                 specification.
 */
#define OBERON_LINUX_VK_DECLARE_PFN(loader, function) \
  auto function = reinterpret_cast<PFN_##function>((loader).get(vkfl::command::function))

#if OBERON_CHECKS_ENABLED
  /**
   * @def OBERON_LINUX_VK_SUCCEEDS
   * @brief Check if the provided Vulkan command succeeds (i.e., returns VK_SUCCESS.
   * @details This may be disabled with other checks by disabling OBERON_CHECKES_ENABLED.
   * @param vkexp The Vulkan command to execute.
   * @throws vk_error If (vkexp) does not return VK_SUCCESS.
   */
  #define OBERON_LINUX_VK_SUCCEEDS(vkexp) \
    do \
    { \
      if (auto res = (vkexp); res != VK_SUCCESS) \
      { \
        throw oberon::linux::vk_error{ "Vulkan command \"" #vkexp "\" failed.", res }; \
      } \
    } \
    while (0)
#else
  #define OBERON_LINUX_VK_SUCCEEDS(vkexp) \
    ((void) (vkexp))
#endif

namespace oberon::linux {

  OBERON_DYNAMIC_EXCEPTION_TYPE(vk_error);

  VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_log(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                                              const VkDebugUtilsMessengerCallbackDataEXT*, void*);

}

#endif
