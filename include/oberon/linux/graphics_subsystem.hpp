#ifndef OBERON_LINUX_GRAPHICS_SUBSYSTEM_HPP
#define OBERON_LINUX_GRAPHICS_SUBSYSTEM_HPP

#include <vector>

#include "../memory.hpp"
#include "../errors.hpp"
#include "../graphics_subsystem.hpp"

#include "vulkan.hpp"

namespace oberon::linux {

  class io_subsystem;

  class graphics_subsystem final : public abstract_graphics_subsystem {
  private:
    vkfl::loader m_vkdl{ vkGetInstanceProcAddr };
    VkDebugUtilsMessengerEXT m_debug_messenger{ };
    std::vector<VkPhysicalDevice> m_available_physical_devices{ };
    VkPhysicalDevice m_physical_device{ };
    u32 m_primary_queue_family{ };
    VkQueue m_primary_queue{ };

    void open_vk_instance(const readonly_ptr<cstring> layers, const u32 layer_count,
                          const readonly_ptr<cstring> extensions, const u32 extension_count,
                          const ptr<void> next);
    void open_vk_debug_utils_messenger(const VkDebugUtilsMessengerCreateInfoEXT& debug_info);
    void open_vk_physical_devices(io_subsystem& io);
    void open_vk_selected_physical_device(const u32 index);
    void open_vk_device(const readonly_ptr<cstring> extensions, const u32 extension_count, const ptr<void> next);
    void open_vk_device_nvidia(VkDeviceCreateInfo& info);
    void open_vk_device_amd(VkDeviceCreateInfo& info);
    void open_vk_device_intel(VkDeviceCreateInfo& info);
    void open_vk_device_generic(VkDeviceCreateInfo& info);
    void open_vk_device_common(VkDeviceCreateInfo& info);
    void close_vk_device() noexcept;
    void close_vk_selected_physical_device() noexcept;
    void close_vk_physical_devices() noexcept;
    void close_vk_debug_utils_messenger() noexcept;
    void close_vk_instance() noexcept;
  public:
    graphics_subsystem(io_subsystem& io, const u32 device_index, const bool create_debug_instance);

    ~graphics_subsystem() noexcept;

    constexpr subsystem_implementation implementation() noexcept override {
      return subsystem_implementation::vulkan;
    }

    const vkfl::loader& vk_loader();
    VkInstance vk_instance();
    VkPhysicalDevice vk_physical_device();
    u32 vk_primary_queue_family() const;
    VkDevice vk_device();
    VkQueue vk_primary_queue();
  };

  static_assert(is_graphics_subsystem_v<graphics_subsystem>);

  OBERON_EXCEPTION_TYPE(vk_create_instance_failed, "Failed to create Vulkan instance.", 1);
  OBERON_EXCEPTION_TYPE(vk_create_debug_messenger_failed, "Failed to create Vulkan debug messenger.", 1);
  OBERON_EXCEPTION_TYPE(vk_enumerate_physical_devices_failed, "Failed to enumerate Vulkan physical devices.", 1);
  OBERON_EXCEPTION_TYPE(vk_no_such_physical_device,
                        "No Vulkan physical device meeting the system requirements is available.", 1);
  OBERON_EXCEPTION_TYPE(vk_bad_device_index, "The client provided Vulkan device index is out of bounds.", 1);
  OBERON_EXCEPTION_TYPE(vk_create_device_failed, "Failed to create Vulkan device", 1);

}

#endif
