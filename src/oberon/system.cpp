#include "oberon/system.hpp"

#include <utility>

#include "oberon/internal/system.hpp"
#include "oberon/internal/wsi_system.hpp"

namespace oberon {

  system::system(ptr<internal::system>&& impl) : m_impl{ std::exchange(impl, nullptr) } { }

  system::implementation_reference system::internal() {
    return *m_impl;
  }

  wsi_system::wsi_system(ptr<internal::wsi_system>&& impl) :
  system{ std::move(static_cast<ptr<internal::system>>(impl)) } { }


}
