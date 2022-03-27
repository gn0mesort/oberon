#ifndef OBERON_SYSTEM_HPP
#define OBERON_SYSTEM_HPP

#include "memory.hpp"

namespace oberon::detail {
  struct system_impl;
  struct system_impl_dtor final {
  void operator()(const ptr<system_impl> p) const noexcept;
  };
}

namespace oberon {

  class context;

  class system final {
  public:
    enum class hint {
      // Generic
      none = 0 /* void */,
      debug_context /* bool */,
      // Linux
      x_display /* cstring */,
      vulkan_device_index /* u32 */
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

}

#endif
