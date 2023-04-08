#ifndef OBERON_INTERNAL_SYSTEM_HPP
#define OBERON_INTERNAL_SYSTEM_HPP

namespace oberon::internal {

  class system {
  public:
    inline virtual ~system() noexcept = 0;
  };

  system::~system() noexcept { }

}

#endif
