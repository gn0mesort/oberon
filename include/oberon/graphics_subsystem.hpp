#ifndef OBERON_GRAPHICS_SUBSYSTEM_HPP
#define OBERON_GRAPHICS_SUBSYSTEM_HPP

#include "types.hpp"
#include "vulkan.hpp"

namespace oberon {

  class io_subsystem;

  class graphics_subsystem final {
  private:
    vkfl::loader m_vkdl{ vkGetInstanceProcAddr };
    VkInstance m_instance{ };
    VkPhysicalDevice m_physical_device{ };
    VkPhysicalDeviceProperties m_physical_device_properties{ };
    u32 m_primary_queue_family{ };
    VkDevice m_device{ };
    VkQueue m_primary_queue{ };
  public:
    graphics_subsystem(io_subsystem& io);
  };

}

#endif
