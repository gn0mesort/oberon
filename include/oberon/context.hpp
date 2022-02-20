#ifndef OBERON_CONTEXT_HPP
#define OBERON_CONTEXT_HPP

#include "object.hpp"

namespace oberon {
namespace detail {

  class x11_connection_xtor;
  class vk_instance_xtor;
  class vk_device_xtor;

  struct context_impl;
}

  struct event;

  class context : public object {
  private:
    void v_dispose() noexcept override final;
  protected:
    context(const ptr<detail::context_impl> child_impl, const detail::x11_connection_xtor& x11,
            const detail::vk_instance_xtor& vk_instance, const detail::vk_device_xtor& vk_device);
  public:
    context(const std::u8string& application_name, const u16 application_version_major,
            const u16 application_version_minor, const u16 application_version_patch);

    virtual ~context() noexcept;

    const std::string& application_name() const;

    bool poll_events(event& ev);
  };

}

#endif
