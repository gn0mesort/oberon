#ifndef OBERON_GRAPHICS_SUBSYSTEM_HPP
#define OBERON_GRAPHICS_SUBSYSTEM_HPP

#include "basics.hpp"
#include "vulkan.hpp"

namespace oberon {

  class io_subsystem;

  class graphics_subsystem final {
  private:
    vkfl::loader m_vkdl{ vkGetInstanceProcAddr };
    VkInstance m_instance{ };
    VkDebugUtilsMessengerEXT m_debug_messenger{ };
    VkPhysicalDevice m_physical_device{ };
    VkPhysicalDeviceProperties m_physical_device_properties{ };
    u32 m_primary_queue_family{ };
    VkDevice m_device{ };
    VkQueue m_primary_queue{ };

    void open_vk_instance();
    void open_vk_device(io_subsystem& io, const u32 device_index);
    void close_vk_device() noexcept;
    void close_vk_instance() noexcept;
  public:
    graphics_subsystem(io_subsystem& io, const u32 device_index);
    graphics_subsystem(const graphics_subsystem& other) = delete;
    graphics_subsystem(graphics_subsystem&& other) = delete;

    ~graphics_subsystem() noexcept;

    graphics_subsystem& operator=(const graphics_subsystem& rhs) = delete;
    graphics_subsystem& operator=(graphics_subsystem&& rhs) = delete;
  };

  OBERON_STATIC_EXCEPTION_TYPE(no_device_found, "No Vulkan device corresponding to the desired index was found.", 1);

}

#endif
