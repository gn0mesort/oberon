#ifndef OBERON_SYSTEM_HPP
#define OBERON_SYSTEM_HPP

#include "types.hpp"
#include "memory.hpp"

/*
namespace oberon::detail {
  struct system_impl;
  struct system_impl_dtor final {
  void operator()(const ptr<system_impl> p) const noexcept;
  };
}
*/

namespace oberon {

  class system {
  public:
    static constexpr uptr bad_parameter{ static_cast<uptr>(-1) };

    inline virtual ~system() noexcept = 0;

    virtual void set_parameter(const umax param, const uptr value) = 0;
    virtual uptr get_parameter(const umax param) const = 0;
  };

  system::~system() noexcept { }

  class onscreen_context;
  class render_window;

  class onscreen_system : public system {
  public:
    using entry_point = int(system&, onscreen_context&, render_window&);

    inline virtual ~onscreen_system() noexcept = 0;

    virtual int run(const ptr<entry_point> main) = 0;
  };

  onscreen_system::~onscreen_system() noexcept { }

  class offscreen_context;
  class render_image;

  class offscreen_system : public system {
  public:
    using entry_point = int(system&, offscreen_context&, render_image&);

    inline virtual ~offscreen_system() noexcept = 0;

    virtual int run(const ptr<entry_point> main) = 0;
  };

  offscreen_system::~offscreen_system() noexcept { }

  /*
  class system final {
  public:
    enum class hint {
      // Generic
      none = 0, // void
      debug_context, // bool
      // Linux
      x_display, // cstring
      vulkan_device_index // u32
    };

    static constexpr uptr bad_hint = uptr{ static_cast<uptr>(-1) };
  private:
    std::unique_ptr<detail::system_impl, detail::system_impl_dtor> m_impl{ };
  public:
    using entry_point = int(system&, context&);

    system();
    system(system&& other) = default;

    ~system() noexcept = default;

    system& operator=(system&& rhs) = default;

    void set_hint(const hint ht, const uptr value);
    uptr get_hint(const hint ht) const;
    int run(const ptr<entry_point> main);
  };
  */

}

#endif
