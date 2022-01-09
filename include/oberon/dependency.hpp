#ifndef OBERON_DEPENDENCY_HPP
#define OBERON_DEPENDENCY_HPP

#include "memory.hpp"

namespace oberon {

  template <typename Type>
  class dependency final {
  public:
    using value_type = Type;
  private:
    ptr<value_type> m_value{ };
  public:
    dependency(value_type& value);
    dependency(const dependency& other) = default;
    dependency(dependency&& other) = default;

    dependency& operator=(const dependency& rhs) = default;
    dependency& operator=(dependency&& rhs) = default;

    value_type& value();
    const value_type& value() const;
  };


  template <typename Type>
  dependency<Type>::dependency(value_type& value) : m_value{ &value } { }

  template <typename Type>
  typename dependency<Type>::value_type& dependency<Type>::value() {
    return *m_value;
  }

  template <typename Type>
  const typename dependency<Type>::value_type& dependency<Type>::value() const {
    return *m_value;
  }

}

#endif
