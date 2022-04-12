#ifndef OBERON_LINUX_RENDER_SYSTEM_HPP
#define OBERON_LINUX_RENDER_SYSTEM_HPP

#include "../types.hpp"
#include "../memory.hpp"
#include "../errors.hpp"

namespace oberon::linux::detail {

  OBERON_NON_OWNING_PIMPL_FWD(render_system);

}

namespace oberon::linux {

  class render_system final {
  private:
    friend class application;
    friend class render_window;

    OBERON_NON_OWNING_PIMPL_PTR(detail, render_system);

    render_system(const ptr<detail::render_system_impl> impl);

    ~render_system() noexcept = default;
  public:
    render_system(const render_system& other) = delete;
    render_system(render_system&& other) = delete;

    render_system& operator=(const render_system& rhs) = delete;
    render_system& operator=(render_system&& rhs) = delete;


    uptr device_handle() const;
    void change_device(const u32 index);
  };

  OBERON_EXCEPTION_TYPE(vk_instance_create_failed, "Failed to create Vulkan instance.", 1);
  OBERON_EXCEPTION_TYPE(vk_debug_messenger_create_failed, "Failed to create Vulkan debug messenger.", 1);
  OBERON_EXCEPTION_TYPE(vk_physical_device_enumeration_failed, "Failed to enumerate Vulkan physical devices.", 1);
  OBERON_EXCEPTION_TYPE(vk_no_such_physical_device, "The desired Vulkan physical device index is out of bounds.", 1);
  OBERON_EXCEPTION_TYPE(vk_device_create_failed, "Failed to create Vulkan device.", 1);

}

#endif
