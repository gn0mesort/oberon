#ifndef OBERON_INTERNAL_VULKAN_SYSTEM_HPP
#define OBERON_INTERNAL_VULKAN_SYSTEM_HPP

#include <unordered_set>
#include <string>

#include "../memory.hpp"

#include "system.hpp"
#include "vulkan_graphics_context.hpp"

namespace oberon::internal {


  class vulkan_system : public system {
  protected:
    struct defer_graphics_device_construction final { };

    ptr<vulkan_graphics_context> m_graphics_context{ };

    vulkan_system(const std::unordered_set<std::string>& requested_layers, const defer_graphics_device_construction&);
  public:
    vulkan_system(const std::unordered_set<std::string>& requested_layers);
    virtual ~vulkan_system() noexcept;
  };

}

#endif
