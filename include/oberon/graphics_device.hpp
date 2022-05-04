#ifndef OBERON_GRAPHICS_DEVICE_HPP
#define OBERON_GRAPHICS_DEVICE_HPP

#include <string>

namespace oberon {

  class abstract_graphics_device {
  protected:
    abstract_graphics_device() = default;

    virtual ~abstract_graphics_device() noexcept = default;
  public:
    virtual std::string_view name() const = 0;
  };

}

#endif
