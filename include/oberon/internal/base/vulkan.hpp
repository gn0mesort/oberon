/**
 * @file vulkan.hpp
 * @brief Internal Vulkan header.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_BASE_VULKAN_HPP
#define OBERON_INTERNAL_BASE_VULKAN_HPP

#include <limits>

#include "configuration.hpp"

#define VK_NO_PROTOTYPES 1
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#if defined(CONFIGURATION_OPERATING_SYSTEM_LINUX) && defined (CONFIGURATION_WINDOW_SYSTEM_X11)
  #include <xcb/xcb.h>
  #include <vulkan/vulkan_xcb.h>
#endif

#include <vk_mem_alloc.h>

#include <vkfl.hpp>

#include "../../types.hpp"
#include "../../errors.hpp"

/**
 * @def OBERON_INTERNAL_BASE_VK_STRUCT(name)
 * @brief Alias macro for shortening VK_STRUCTURE_TYPE names.
 */
#define OBERON_INTERNAL_BASE_VK_STRUCT(name) (VK_STRUCTURE_TYPE_##name)

/**
 * @def OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, pfn)
 * @brief Declare the Vulkan command `pfn` using the loader `dl`.
 */
#define OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, pfn) \
  auto pfn = (reinterpret_cast<PFN_##pfn>((dl).get(vkfl::command::pfn)))

/**
 * @def OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp)
 * @brief Check if `exp` returns `VK_SUCCESS`.
 * @details If `exp` fails then this throws an error.
 */
#define OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp) \
  do \
  { \
    auto result = (exp); \
    OBERON_CHECK_ERROR_MSG(result == VK_SUCCESS, result, "Vulkan command \"" #exp "\" failed with value \"%u\" (%x)", \
                           result, result); \
  } \
  while (0)

/// @cond
// This provides a static declaration of vkGetInstanceProcAddr for use with vkfl::loader.
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char*);
/// @endcond

/**
 * @brief A callback that will be invoked when a Vulkan debug message is logged.
 * @details vkDebugUtilsMessengerCallbackEXT *can* be called from multiple threads simultaneously. Therefore it is
 *          crucial that this function be implemented in a thread-safe way.
 * @param messageSeverity The message severity.
 * @param messageTypes The types of message that the message belongs to.
 * @param pCallbackData The message data.
 * @param pUserData A pointer to a user specified data structure.
 * @return `VK_FALSE` in all cases.
 */
extern "C" VKAPI_ATTR VkBool32 VKAPI_CALL
vkDebugUtilsMessengerCallbackEXT(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                 VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
enum {
  /**
   * @var VK_FOREVER
   * @brief A constant representing an indefinite wait time.
   */
  VK_FOREVER = std::numeric_limits<oberon::u64>::max()
};

#endif
