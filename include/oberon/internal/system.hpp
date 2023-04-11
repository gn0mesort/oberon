#ifndef OBERON_INTERNAL_SYSTEM_HPP
#define OBERON_INTERNAL_SYSTEM_HPP

#include <vector>
#include <unordered_set>
#include <string>

#include "../system.hpp"
#include "../memory.hpp"
#include "../graphics_device.hpp"

namespace oberon::internal {

  class graphics_device;
  class graphics_context;

  class system {
  protected:
    struct defer_construction final { };

    std::unique_ptr<graphics_context> m_graphics_context{ };
    std::vector<std::unique_ptr<graphics_device>> m_graphics_device_ptrs{ };
    std::vector<oberon::graphics_device> m_graphics_devices{ };

    system(const defer_construction&);
  public:
    system(const std::unordered_set<std::string>& requested_layers);
    system(const system& other) = delete;
    system(system&& other) = delete;

    virtual ~system() noexcept;

    system& operator=(const system& rhs) = delete;
    system& operator=(system&& rhs) = delete;

    const std::vector<oberon::graphics_device>& graphics_devices();
  };

}

#endif
