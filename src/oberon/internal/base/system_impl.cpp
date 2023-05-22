/**
 * @file system_impl.cpp
 * @brief Internal system API implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/internal/base/system_impl.hpp"

#include "oberon/system.hpp"

#include "oberon/internal/base/graphics_context.hpp"

namespace oberon::internal::base {

  void system_impl_dtor::operator()(const ptr<system_impl> p) const noexcept {
    delete p;
  }

  system_impl::system_impl(ptr<graphics_context>&& gfx) : m_graphics_context{ std::exchange(gfx, nullptr) } { }

  system_impl::~system_impl() noexcept {
    delete m_graphics_context;
  }

}
