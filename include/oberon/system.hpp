#ifndef OBERON_SYSTEM_HPP
#define OBERON_SYSTEM_HPP

#include <string>
#include <vector>
#include <unordered_set>

#include "types.hpp"
#include "memory.hpp"
#include "implementation_owner.hpp"

namespace oberon::internal {
  OBERON_OPAQUE_BASE_FWD(system);
}

namespace oberon {

  class system final {
  public:
    using implementation_type = internal::system;
    using implementation_reference = implementation_type&;
  private:
    friend class application;

    OBERON_OPAQUE_BASE_PTR(internal::system);

    system(const ptr<internal::system> impl);
  public:
    system(const system& other) = delete;
    system(system&& other) = delete;

    ~system() noexcept = default;

    system& operator=(const system& rhs) = delete;
    system& operator=(system&& rhs) = delete;

    implementation_reference internal();
  };

  OBERON_ENFORCE_CONCEPT(implementation_owner, system);

}

#endif
