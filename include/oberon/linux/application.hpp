#ifndef OBERON_LINUX_APPLICATION_HPP
#define OBERON_LINUX_APPLICATION_HPP

#include "../types.hpp"
#include "../memory.hpp"

namespace oberon::linux {

  class window_system;
  class render_system;

  class application {
  private:
    ptr<void> m_user_data{ };
    bool m_vulkan_create_debug_context{ };
    u32 m_vulkan_device{ };
  public:
    using entry_point = int(window_system& win, render_system& rnd, const ptr<void> user);

    virtual ~application() noexcept = default;

    void set_user_data(const ptr<void> user);
    void set_vulkan_debug_flag(const bool flag);
    void set_vulkan_device(const u32 index);

    int run(const ptr<entry_point> main) const;
  };

}

#endif
