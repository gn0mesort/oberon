#ifndef OBERON_SYSTEM_HPP
#define OBERON_SYSTEM_HPP

#include <span>

#include "memory.hpp"

#include "concepts/has_internal_implementation.hpp"

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(system_impl);

}

namespace oberon {

  class graphics_device;

  class system final {
  public:
    using implementation_type = internal::base::system_impl;
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::system_impl);
  public:
    system(ptr<implementation_type>&& impl);
    system(const system& other) = delete;
    system(system&& other) = delete;

    ~system() noexcept = default;

    system& operator=(const system& rhs) = delete;
    system& operator=(system&& rhs) = delete;

    implementation_type& implementation();

    std::span<graphics_device> graphics_devices();
    graphics_device& preferred_graphics_device();
  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, system);

}

#endif
