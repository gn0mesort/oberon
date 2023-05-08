#ifndef OBERON_APPLICATION_HPP
#define OBERON_APPLICATION_HPP

#include <functional>

#include "types.hpp"
#include "memory.hpp"

namespace oberon {

  class system;

  class application final {
  public:
    using entry_point = int(const i32, const ptr<csequence>, system&);

    int run(const std::function<entry_point>& fn, const i32 argc, const ptr<csequence> argv);
  };

}

#endif
