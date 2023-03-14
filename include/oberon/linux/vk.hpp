/**
 * @file vk.hpp
 * @brief Vulkan support header.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_LINUX_VK_HPP
#define OBERON_LINUX_VK_HPP

#include <X11/Xlib-xcb.h>

/// @cond
#define VK_NO_PROTOTYPES 1
/// @endcond

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_xcb.h>

/// @cond
#undef VK_NO_PROTOTYPES
/// @endcond


#include <vkfl.hpp>

#include "../types.hpp"
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

  /**
   * @brief An object representing Vulkan errors.
   */
  OBERON_DYNAMIC_EXCEPTION_TYPE(vk_error);

  /**
   * @brief Vulkan debug logging function for VK_EXT_debug_utils.
   * @param severity The severity of the message.
   * @param messageTypes The types that the message belongs to.
   * @param pCallbackData The message data.
   * @param pUserData A user data item as provided to vkCreateDebugUtilsMessengerEXT.
   */
  VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_log(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                                              const VkDebugUtilsMessengerCallbackDataEXT*, void*);

  /**
   * @brief AMD PCI vendor ID.
   */
  constexpr const u32 OBERON_LINUX_VK_PCI_VENDOR_ID_AMD{ 0x1002 };

  /**
   * @brief Nvidia PCI vendor ID.
   */
  constexpr const u32 OBERON_LINUX_VK_PCI_VENDOR_ID_NVIDIA{ 0x10de };

  /**
   * @brief Intel PCI vendor ID.
   */
  constexpr const u32 OBERON_LINUX_VK_PCI_VENDOR_ID_INTEL{ 0x8086 };

  /**
   * @brief The maximum number of frames to render before waiting for a render step to complete.
   * @details This should be a power of 2.
   */
  constexpr const usize OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT{ 2 };

  /**
   * @brief The value that Vulkan uses to represent an infinite wait timeout.
   */
  constexpr const auto OBERON_LINUX_VK_FOREVER = std::numeric_limits<u64>::max();
}

#endif
