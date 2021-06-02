#ifndef OBERON_DETAIL_GRAPHICS_HPP
#define OBERON_DETAIL_GRAPHICS_HPP

#include <vector>
#include <unordered_map>
#include <iterator>
#include <optional>

#include <xcb/xcb.h>

// clangd doesn't like it if I omit this.
#define VK_USE_PLATFORM_XCB_KHR 1
#include <vulkan/vulkan.hpp>

#include "../memory.hpp"
#include "../errors.hpp"

#define VK_GLOBAL_FN(name) \

namespace oberon {
namespace detail {
  ptr<xcb_screen_t> screen_of_display(const ptr<xcb_connection_t>, const int screen);

  template <typename FunctionPointer>
  FunctionPointer vk_load_fn(const cstring name) {
    auto pfn = reinterpret_cast<FunctionPointer>(vkGetInstanceProcAddr(nullptr, name));
    if (!pfn)
    {
      throw fatal_error{ "Failed to load global function: \"" + std::string{ name } + "\"." }; 
    }
    return pfn;
  }

  template <typename FunctionPointer>
  FunctionPointer vk_load_fn(const VkInstance instance, const cstring name) {
    auto pfn = reinterpret_cast<FunctionPointer>(vkGetInstanceProcAddr(instance, name));
    if (!pfn)
    {
      throw fatal_error{ "Failed to load instance function: \"" + std::string{ name } + "\"." }; 
    }
    return pfn;
  }

  template <typename FunctionPointer>
  FunctionPointer vk_load_fn(const VkInstance instance, const VkDevice device, const cstring name) {
    auto loader = vk_load_fn<PFN_vkGetDeviceProcAddr>(instance, "vkGetDeviceProcAddr");
    auto pfn = reinterpret_cast<FunctionPointer>(loader(device, name));
    if (!pfn)
    {
      throw fatal_error{ "Failed to load device function: \"" + std::string{ name } + "\"." };
    }
    return pfn;
  }

  template <typename ForwardIt, typename Dispatch = VULKAN_HPP_DEFAULT_DISPATCHER>
  std::unordered_map<vk::PhysicalDeviceType, std::vector<vk::PhysicalDevice>> organize_physical_devices(
    ForwardIt begin, ForwardIt end, const Dispatch& dl
  ) {
    auto result = std::unordered_map<vk::PhysicalDeviceType, std::vector<vk::PhysicalDevice>>{ };
    result[vk::PhysicalDeviceType::eCpu] = std::vector<vk::PhysicalDevice>{ };
    result[vk::PhysicalDeviceType::eDiscreteGpu] = std::vector<vk::PhysicalDevice>{ };
    result[vk::PhysicalDeviceType::eIntegratedGpu] = std::vector<vk::PhysicalDevice>{ };
    result[vk::PhysicalDeviceType::eOther] = std::vector<vk::PhysicalDevice>{ };
    result[vk::PhysicalDeviceType::eVirtualGpu] = std::vector<vk::PhysicalDevice>{ };
    for (auto cur = begin; cur != end; cur = std::next(cur))
    {
      result[cur->getProperties(dl).deviceType].push_back(*cur);
    }
    auto cmp = [&dl](const vk::PhysicalDevice& a, const vk::PhysicalDevice& b) {
      auto mem_a = a.getMemoryProperties(dl);
      auto mem_b = b.getMemoryProperties(dl);
      auto total_a = u32{ 0 };
      auto total_b = u32{ 0 };
      // Not sure this is reliable.
      // On my Nvidia GPU this is a valid way to approximate available device memory.
      for (auto i = u32{ 0 }; i < mem_a.memoryHeapCount; ++i)
      {
        auto is_local = u32{ mem_a.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal };
        total_a += mem_a.memoryHeaps[i].size * is_local;
      }
      for (auto i = u32{ 0 }; i < mem_b.memoryHeapCount; ++i)
      {
        auto is_local = u32{ mem_b.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal };
        total_b += mem_b.memoryHeaps[i].size * is_local;
      }
      return total_a < total_b;
    };
    for (auto& [key, value] : result)
    {
      std::sort(std::begin(value), std::end(value), cmp);
    }
    return result;
  }
}
}

#endif
