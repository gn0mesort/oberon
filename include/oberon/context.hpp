#ifndef OBERON_CONTEXT_HPP
#define OBERON_CONTEXT_HPP

#include "memory.hpp"
#include "io_subsystem.hpp"
#include "graphics_subsystem.hpp"

namespace oberon {

  class context;

}

namespace oberon::detail {

  struct subsystem_storage final {
    ptr<abstract_io_subsystem> io{ };
    ptr<abstract_graphics_subsystem> gfx{ };
  };

  class subsystem_factory {
  private:
    virtual void v_create_subsystems(detail::subsystem_storage& subsystems, const readonly_ptr<void> config) = 0;
    virtual void v_destroy_subsystems(detail::subsystem_storage& subsystems) noexcept = 0;
  public:
    subsystem_factory() = default;
    subsystem_factory(const subsystem_factory& other) = delete;
    subsystem_factory(subsystem_factory&& other) = delete;

    virtual ~subsystem_factory() noexcept = default;

    subsystem_factory& operator=(const subsystem_factory& rhs) = delete;

    ptr<context> create_context(const readonly_ptr<void> config);
    void destroy_context(const ptr<context> ctx) noexcept;
  };

}

namespace oberon {

  class context final {
  private:
    friend class detail::subsystem_factory;

    detail::subsystem_storage m_subsystems{ };

    context() = default;
    context(const context& other) = delete;

    context& operator=(const context& rhs) = delete;
  public:
    context(context&& other) = default;

    ~context() noexcept = default;

    context& operator=(context&& rhs) = default;

    abstract_subsystem& get_subsystem(const subsystem_type type);

    abstract_io_subsystem& get_io_subsystem();
    abstract_graphics_subsystem& get_graphics_subsystem();
  };

}

#endif
