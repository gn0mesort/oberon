#ifndef OBERON_INTERNAL_BASE_SYSTEM_IMPL_HPP
#define OBERON_INTERNAL_BASE_SYSTEM_IMPL_HPP

#include <span>

#include "../../memory.hpp"

namespace oberon {

  class graphics_device;

}

namespace oberon::internal::base {

  class graphics_context;

  class system_impl {
  protected:
    ptr<graphics_context> m_graphics_context{ };

    system_impl(ptr<graphics_context>&& gfx);
  public:
    system_impl(const system_impl& other) = delete;
    system_impl(system_impl&& other) = delete;

    virtual ~system_impl();

    system_impl& operator=(const system_impl& rhs) = delete;
    system_impl& operator=(system_impl&& rhs) = delete;

    virtual std::span<graphics_device> graphics_devices() = 0;
  };

}

#endif
