#ifndef OBERON_INTERNAL_BASE_TRANSIENT_OBJECT_POOL
#define OBERON_INTERNAL_BASE_TRANSIENT_OBJECT_POOL

#include <new>

#include "../../types.hpp"
#include "../../memory.hpp"
#include "../../debug.hpp"
#include "../../errors.hpp"
#include "../../utility.hpp"

namespace oberon::internal::base {

  template <typename Base, typename... Derived>
  class transient_object_pool final {
    static_assert(!sizeof...(Derived) || all_derived<Base, Derived...>());
  private:
    ptr<char> m_memory{ };
    usize m_offset{ };
    usize m_elements_in_use{ };
    usize m_size{ };
  public:
    using generic_type = Base;

    static consteval usize element_size() {
      return max_sizeof<Base, Derived...>();
    }

    transient_object_pool(const usize elements) : m_size{ elements * element_size() } {
      if (m_size)
      {
        m_memory = new char[m_size];
        OBERON_CHECK_ERROR_MSG(m_memory, 1, "Failed to allocate %zu bytes", m_size);
      }
    }
    transient_object_pool(const transient_object_pool& other) = delete;
    transient_object_pool(transient_object_pool&& other) = delete;

    ~transient_object_pool() noexcept {
      delete[] m_memory;
    }

    transient_object_pool& operator=(const transient_object_pool& rhs) = delete;
    transient_object_pool& operator=(transient_object_pool&& rhs) = delete;

    void reset() {
      OBERON_CHECK_ERROR_MSG(!m_elements_in_use, 1, "Failed to reset the pool. Memory is still allocated.");
      m_offset = 0;
    }

    template <typename Type, typename... Args>
    ptr<generic_type> construct(Args&&... args) {
      static_assert(std::is_base_of_v<generic_type, Type>);
      static_assert(is_one_of<Type, Base, Derived...>());
      OBERON_CHECK_ERROR_MSG(m_offset + element_size() < m_size, 1, "Failed to construct an object. The pool is "
                             "full.");
      auto res = static_cast<ptr<generic_type>>(::new (m_memory + m_offset) Type{ std::forward<Args>(args)... });
      m_offset += element_size();
      ++m_elements_in_use;
      return res;
    }

    template <typename Type>
    void destroy(ptr<Type> p) noexcept {
      static_assert(std::is_base_of_v<generic_type, Type>);
      static_assert(is_one_of<Type, Base, Derived...>());
      if (p)
      {
        OBERON_ASSERT(p >= m_memory || p < (m_memory + m_size));
        p->~Type();
        --m_elements_in_use;
      }
    }
  };

}

#endif
