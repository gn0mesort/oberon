#ifndef OBERON_GRAPHICS_DEVICE_HPP
#define OBERON_GRAPHICS_DEVICE_HPP

#include "memory.hpp"
#include "implementation_owner.hpp"

namespace oberon::internal {

  class graphics_device;
  class wsi_graphics_device;

}

namespace oberon {

  class graphics_device {
  public:
    using implementation_type = internal::graphics_device;
    using implementation_reference = implementation_type&;
  private:
    ptr<internal::graphics_device> m_impl{ };
  public:
    graphics_device(const ptr<internal::graphics_device> impl);
    graphics_device(const graphics_device& other) = delete;
    graphics_device(graphics_device&& other) = default;

    virtual ~graphics_device() noexcept = default;

    graphics_device& operator=(const graphics_device& rhs) = delete;
    graphics_device& operator=(graphics_device&& rhs) = default;

    implementation_reference internal();
    // If the graphics device is empty only the following methods are defined:
    //  operator=(graphics_device&&)
    //  ~graphics_device() noexcept
    //  empty() const
    bool empty() const;
  };

  OBERON_ENFORCE_CONCEPT(implementation_owner, graphics_device);

  class wsi_graphics_device final : public graphics_device {
  public:
    wsi_graphics_device(const ptr<internal::wsi_graphics_device> impl);
    wsi_graphics_device(const wsi_graphics_device& other) = delete;
    wsi_graphics_device(wsi_graphics_device&& other) = default;

    ~wsi_graphics_device() noexcept = default;

    wsi_graphics_device& operator=(const wsi_graphics_device& rhs) = delete;
    wsi_graphics_device& operator=(wsi_graphics_device&& rhs) = default;
  };


}

#endif
