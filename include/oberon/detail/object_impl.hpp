#ifndef OBERON_DETAIL_OBJECT_IMPL_HPP
#define OBERON_DETAIL_OBJECT_IMPL_HPP

#include "../object.hpp"

namespace oberon {
namespace detail {

  struct object_impl {
    inline virtual ~object_impl() noexcept = 0;
  };

  object_impl::~object_impl() noexcept { }

}
}

#endif
