#ifndef OBERON_DEPENDENCY_STORE_HPP
#define OBERON_DEPENDENCY_STORE_HPP

#include <array>

#include "../types.hpp"
#include "../memory.hpp"

#define OBERON_DETAIL_DECLARE_DEPENDENCY_ENUM(...) \
  enum class dependency_types {\
    __VA_ARGS__ __VA_OPT__(,)\
    max\
  };

#define OBERON_DETAIL_DECLARE_DEPENDENCY_SETTERS(type, ...) \
  OBERON_DETAIL_DECLARE_DEPENDENCY_SETTERS_INNER(type)\
  __VA_OPT__(OBERON_DETAIL_DECLARE_DEPENDENCY_SETTERS(__VA_ARGS__))

#define OBERON_DETAIL_DECLARE_DEPENDENCY_RESOLVER(access, ...) \
  private:\
    OBERON_DETAIL_DECLARE_DEPENDENCY_ENUM(__VA_ARGS__)\
    template <typename Type>\
    void store_dependency(std::enable_if_t<oberon::is_one_of_v<Type, __VA_ARGS__>, Type>& value);\
    oberon::detail::dependency_store<static_cast<oberon::usize>(dependency_types::max)> m_deps{ };\
  access:\
    template <typename Type>\
    std::enable_if_t<oberon::is_one_of_v<Type, __VA_ARGS__>, Type>& dependency();\
    template <typename Type>\
    const std::enable_if_t<oberon::is_one_of_v<Type, __VA_ARGS__>, Type>& dependency() const;

#define OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_BODY_INNER(type) \
  if constexpr (std::is_same_v<Type, type>) {\
    return m_deps.retrieve<Type>(static_cast<oberon::usize>(dependency_types::type));\
  }

#define OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_BODY_1(type, ...) \
  OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_BODY_INNER(type)\
  __VA_OPT__(OBERON_EXPAND(OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_BODY_2(__VA_ARGS__)))

#define OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_BODY_2(type, ...) \
  OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_BODY_INNER(type)\
  __VA_OPT__(OBERON_EXPAND(OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_BODY_1(__VA_ARGS__)))

#define OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_STORE_BODY_INNER(type) \
  if constexpr (std::is_same_v<Type, type>) {\
    m_deps.store(static_cast<oberon::usize>(dependency_types::type), value);\
  }

#define OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_STORE_BODY_1(type, ...) \
  OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_STORE_BODY_INNER(type)\
  __VA_OPT__(OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_STORE_BODY_2(__VA_ARGS__))

#define OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_STORE_BODY_2(type, ...) \
  OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_STORE_BODY_INNER(type)\
  __VA_OPT__(OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_STORE_BODY_1(__VA_ARGS__))

#define OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER(type, ...) \
  template <typename Type>\
  std::enable_if_t<oberon::is_one_of_v<Type, __VA_ARGS__>, Type>& type::dependency() {\
    OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_BODY_1(__VA_ARGS__)\
  }\
  \
  template <typename Type>\
  const std::enable_if_t<oberon::is_one_of_v<Type, __VA_ARGS__>, Type>& type::dependency() const {\
    OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_BODY_1(__VA_ARGS__)\
  }\
  template <typename Type>\
  void type::store_dependency(std::enable_if_t<oberon::is_one_of_v<Type, __VA_ARGS__>, Type>& value) {\
    OBERON_DETAIL_DEFINE_DEPENDENCY_RESOLVER_STORE_BODY_1(__VA_ARGS__)\
  }

namespace oberon {
namespace detail {

  template <usize Size>
  class dependency_store final {
  private:
    std::array<ptr<void>, Size> m_store{ };
  public:
    using size_type = usize;

    static constexpr size_type size() {
      return Size;
    }

    dependency_store() = default;
    dependency_store(const dependency_store& other) = default;
    dependency_store(dependency_store&& other) = default;

    ~dependency_store() noexcept = default;

    dependency_store& operator=(const dependency_store& rhs) = default;
    dependency_store& operator=(dependency_store&& rhs) = default;

    template <typename Type>
    dependency_store& store(const size_type index, Type& value);

    template <typename Type>
    Type& retrieve(const size_type index) const;

    dependency_store& clear(const size_type index);
  };

  template <usize Size>
  template <typename Type>
  dependency_store<Size>& dependency_store<Size>::store(const size_type index, Type& value) {
    m_store[index] = &value;
    return *this;
  }

  template <usize Size>
  template <typename Type>
  Type& dependency_store<Size>::retrieve(const size_type index) const {
    return *reinterpret_cast<Type*>(m_store[index]);
  }

  template <usize Size>
  dependency_store<Size>& dependency_store<Size>::clear(const size_type index) {
    m_store[index] = nullptr;
    return *this;
  }

}
}

#endif
