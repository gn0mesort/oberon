#ifndef OBERON_GRAPHICS_HPP
#define OBERON_GRAPHICS_HPP

#include <string>
#include <vector>

#include "types.hpp"

#define OBERON_GRAPHICS_DEVICE_TYPES \
  OBERON_GRAPHICS_DEVICE_TYPE(other, 0) \
  OBERON_GRAPHICS_DEVICE_TYPE(integrated, 1) \
  OBERON_GRAPHICS_DEVICE_TYPE(discrete, 2) \
  OBERON_GRAPHICS_DEVICE_TYPE(virtualized, 3) \
  OBERON_GRAPHICS_DEVICE_TYPE(cpu, 4)

namespace oberon {

#define OBERON_GRAPHICS_DEVICE_TYPE(name, value) name = (value),
  enum class graphics_device_type {
    OBERON_GRAPHICS_DEVICE_TYPES
  };
#undef OBERON_GRAPHICS_DEVICE_TYPE

  struct graphics_device final {
    graphics_device_type type{ };
    uptr handle{ };
    std::string name{ };
    std::string driver_name{ };
    std::string driver_info{ };
  };

  class graphics {
  public:
    graphics() = default;
    graphics(const graphics& other) = default;
    graphics(graphics&& other) = default;

    inline virtual ~graphics() noexcept = 0;

    graphics& operator=(const graphics& rhs) = default;
    graphics& operator=(graphics&& rhs) = default;


    virtual const std::vector<graphics_device>& available_devices() const = 0;
    virtual void select_device(const graphics_device& device) = 0;
    virtual void wait_for_device_to_idle() = 0;
  };

  graphics::~graphics() noexcept { }

  std::string to_string(const graphics_device_type type);

}

#endif
