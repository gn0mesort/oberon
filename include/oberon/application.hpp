#ifndef OBERON_APPLICATION_HPP
#define OBERON_APPLICATION_HPP

#include "basics.hpp"

namespace oberon {

  class io_subsystem;
  class graphics_subsystem;

  class context final {
  private:
    friend class application;

    ptr<io_subsystem> m_io{ nullptr };
    ptr<graphics_subsystem> m_graphics{ nullptr };

    context(const ptr<io_subsystem> io, const ptr<graphics_subsystem> graphics);

    ~context() noexcept = default;
  public:
    context(const context& other) = delete;
    context(context&& other) = delete;

    context& operator=(const context& other) = delete;
    context& operator=(context&& other) = delete;

    io_subsystem& io();
    graphics_subsystem& graphics();
  };

  class application final {
  private:
  public:
    using entry_point = int(context&);

    application() = default;
    application(const application& other) = delete;
    application(application&& other) = delete;

    ~application() noexcept = default;

    application& operator=(const application& other) = delete;
    application& operator=(application&& other) = delete;

    int run(const ptr<entry_point> application_entry);
  };

}

#endif
