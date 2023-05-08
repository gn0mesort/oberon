#include "oberon/internal/base/system_impl.hpp"

#include "oberon/system.hpp"

#include "oberon/internal/base/graphics_context.hpp"

namespace oberon::internal::base {

  void system_impl_dtor::operator()(const ptr<system_impl> p) const noexcept {
    delete p;
  }

  system_impl::system_impl(ptr<graphics_context>&& gfx) : m_graphics_context{ std::exchange(gfx, nullptr) } { }

  system_impl::~system_impl() {
    delete m_graphics_context;
  }

}
