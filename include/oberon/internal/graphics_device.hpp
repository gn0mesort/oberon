#ifndef OBERON_INTERNAL_GRAPHICS_DEVICE_HPP
#define OBERON_INTERNAL_GRAPHICS_DEVICE_HPP

#include <string>
#include <unordered_set>
#include <vector>

#include "vulkan.hpp"

namespace oberon::internal {

  class graphics_device {
  public:
    static VkPhysicalDeviceProperties2 query_properties(const vkfl::loader& dl, const VkPhysicalDevice physical_device,
                                                        const ptr<void> next);
    static VkPhysicalDeviceFeatures2 query_features(const vkfl::loader& dl, const VkPhysicalDevice physical_device,
                                                    const ptr<void> next);
  protected:
    struct defer_construction final { };

    static VkDeviceQueueCreateInfo pack_device_queue_info(const u32 queue_family, const u32 count,
                                                          const readonly_ptr<f32> priorities);
    static std::unordered_set<std::string> available_extensions(const vkfl::loader& dl,
                                                                const VkPhysicalDevice physical_device);
    static std::vector<cstring> select_extensions(std::unordered_set<std::string>& available_extensions,
                                                  const std::unordered_set<std::string>& required_extensions,
                                                  const std::unordered_set<std::string>& requested_extensions);
    static VkDeviceCreateInfo pack_device_info(const readonly_ptr<VkPhysicalDeviceFeatures> features,
                                               const readonly_ptr<VkDeviceQueueCreateInfo> queue_infos,
                                               const u32 queue_count, const readonly_ptr<cstring> extensions,
                                               const u32 extension_count, const ptr<void> next);
    static void intern_device_in_loader(vkfl::loader& dl, const VkDevice device);
    static VmaAllocatorCreateInfo pack_allocator_info(const vkfl::loader& dl, const VkPhysicalDevice physical_device);

    vkfl::loader m_dl{ vkGetInstanceProcAddr };
    VkPhysicalDevice m_physical_device{ };
    u32 m_graphics_queue_family{ };
    VkQueue m_graphics_queue{ };
    VmaAllocator m_allocator{ };

    VkPhysicalDeviceProperties m_properties{ };
    VkPhysicalDeviceVulkan11Properties m_properties_11{ };
    VkPhysicalDeviceVulkan12Properties m_properties_12{ };
    VkPhysicalDeviceVulkan13Properties m_properties_13{ };
    VkPhysicalDeviceFeatures m_features{ };
    VkPhysicalDeviceVulkan11Features m_features_11{ };
    VkPhysicalDeviceVulkan12Features m_features_12{ };
    VkPhysicalDeviceVulkan13Features m_features_13{ };

    graphics_device(const defer_construction&);
  public:
    graphics_device(const vkfl::loader& dl, const VkPhysicalDevice physical_device, const u32 graphics_queue_family,
                    const std::unordered_set<std::string>& required_extensions,
                    const std::unordered_set<std::string>& requested_extensions);
    graphics_device(const graphics_device& other) = delete;
    graphics_device(graphics_device&& other) = delete;

    virtual ~graphics_device() noexcept;

    graphics_device& operator=(const graphics_device& rhs) = delete;
    graphics_device& operator=(graphics_device&& rhs) = delete;
  };

}

#endif
