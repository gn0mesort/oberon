#ifndef OBERON_GRAPHICS_DEVICE_HPP
#define OBERON_GRAPHICS_DEVICE_HPP

#include <string>

#include "types.hpp"
#include "memory.hpp"

#include "concepts/has_internal_implementation.hpp"

#define OBERON_GRAPHICS_DEVICE_TYPES \
  OBERON_GRAPHICS_DEVICE_TYPE(other, 0) \
  OBERON_GRAPHICS_DEVICE_TYPE(integrated, 1) \
  OBERON_GRAPHICS_DEVICE_TYPE(discrete, 2) \
  OBERON_GRAPHICS_DEVICE_TYPE(virtualized, 3) \
  OBERON_GRAPHICS_DEVICE_TYPE(cpu, 4)

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(graphics_device_impl);

}

namespace oberon {

#define OBERON_GRAPHICS_DEVICE_TYPE(name, value) name = (value),
  enum class graphics_device_type {
    OBERON_GRAPHICS_DEVICE_TYPES
  };
#undef OBERON_GRAPHICS_DEVICE_TYPE

  class graphics_device final {
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::graphics_device_impl);
  public:
    using implementation_type = internal::base::graphics_device_impl;

    graphics_device(ptr<internal::base::graphics_device_impl>&& impl);
    graphics_device(const graphics_device& other) = delete;
    graphics_device(graphics_device&& other) = delete;

    ~graphics_device() noexcept = default;

    graphics_device& operator=(const graphics_device& rhs) = delete;
    graphics_device& operator=(graphics_device&& rhs) = delete;

    implementation_type& implementation();

    graphics_device_type type() const;
    std::string name() const;
    std::string driver_name() const;
    std::string driver_info() const;
    u32 vendor_id() const;
    u32 device_id() const;
    usize total_memory() const;
    std::string uuid() const;

  };

  OBERON_ENFORCE_CONCEPT(has_internal_implementation, graphics_device);

  std::string to_string(const graphics_device_type type);

}

#endif
