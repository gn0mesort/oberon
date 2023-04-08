#ifndef OBERON_IMPLEMENTATION_OWNER_HPP
#define OBERON_IMPLEMENTATION_OWNER_HPP

#include <concepts>

namespace oberon {

  template <typename Type>
  concept implementation_owner = requires (Type t) {
    typename Type::implementation_reference;
    { t.internal() } -> std::same_as<typename Type::implementation_reference>;
  };

}

#endif
