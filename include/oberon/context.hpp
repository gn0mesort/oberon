#ifndef OBERON_CONTEXT_HPP
#define OBERON_CONTEXT_HPP

#include "object.hpp"

namespace oberon {
namespace detail {
  struct context_impl;
}
  class render_window;

  class context : public object {
  private:
    void v_dispose() noexcept override;
  protected:
    context(const ptr<detail::context_impl> child_impl);
  public:
    context(
      const std::string& application_name,
      const u16 application_version_major,
      const u16 application_version_minor,
      const u16 application_version_patch
    );

    virtual ~context() noexcept;
  };

}

#endif
