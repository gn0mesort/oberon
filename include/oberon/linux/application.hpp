#ifndef OBERON_LINUX_APPLICATION_HPP
#define OBERON_LINUX_APPLICATION_HPP

#include "../types.hpp"
#include "../memory.hpp"
#include "../context.hpp"

namespace oberon::linux {

  class application {
  private:
    class subsystem_factory final : public oberon::detail::subsystem_factory {
      void v_create_subsystems(oberon::detail::subsystem_storage& subsystems,
                               const readonly_ptr<void> config) override;
      void v_destroy_subsystems(oberon::detail::subsystem_storage& subsystems) noexcept override;
    };

    struct config final {
      ptr<void> user_data{ };
      struct {
        cstring x_display{ };
      } io;
      struct {
        bool vk_create_debug_instance{ };
        u32 vk_device_index{ };
      } gfx;
    };

    config m_config{ };
  public:
    using entry_point = int(context& ctx, const ptr<void> user);

    virtual ~application() noexcept = default;

    void set_user_data(const ptr<void> user);
    void set_vulkan_debug_flag(const bool flag);
    void set_vulkan_device(const u32 index);

    int run(const ptr<entry_point> main) const;
  };

}

#endif
