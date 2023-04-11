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
  class wsi_system;
}

namespace oberon {

  class system {
  public:
    using implementation_type = internal::system;
    using implementation_reference = implementation_type&;
  private:
    OBERON_OPAQUE_BASE_PTR(internal::system);
  public:
    system(ptr<internal::system>&& impl);
    system(const system& other) = delete;
    system(system&& other) = delete;

    virtual ~system() noexcept = default;

    system& operator=(const system& rhs) = delete;
    system& operator=(system&& rhs) = delete;

    implementation_reference internal();
  };

  OBERON_ENFORCE_CONCEPT(implementation_owner, system);

  class wsi_system final : public system {
  public:
    wsi_system(ptr<internal::wsi_system>&& impl);
    wsi_system(const wsi_system& other) = delete;
    wsi_system(wsi_system&& other) = delete;

    ~wsi_system() noexcept = default;

    wsi_system& operator=(const wsi_system& rhs) = delete;
    wsi_system& operator=(wsi_system&& rhs) = delete;
  };

}

#endif
