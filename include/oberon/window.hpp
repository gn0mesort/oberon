#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include <concepts>

#include "types.hpp"

namespace oberon {

  struct bounding_rect;

  template <typename Type>
  concept window = requires (Type& t) {
    std::destructible<Type>;
    std::movable<Type>;
    !std::copyable<Type>;
  };

  class abstract_window {
  protected:
    abstract_window() = default;
  public:
    abstract_window(const abstract_window& other) = delete;
    abstract_window(abstract_window&& other) = default;

    virtual ~abstract_window() noexcept = default;

    abstract_window& operator=(const abstract_window& rhs) = delete;
    abstract_window& operator=(abstract_window&& rhs) = default;

    virtual void show() = 0;
    virtual void hide() = 0;
    virtual const bounding_rect& bounds() const = 0;
  };

  template <typename Type>
  struct is_window final {
    static constexpr bool value{ inherits_from<Type, abstract_window> && window<Type> };
  };

  template <typename Type>
  constexpr bool is_window_v = is_window<Type>::value;

  static_assert(is_window_v<abstract_window>);

}

#endif
