#ifndef OBERON_APPLICATION_HPP
#define OBERON_APPLICATION_HPP

#include <functional>

#include "types.hpp"
#include "memory.hpp"

namespace oberon {

  class system;

  class application final {
  private:
    bool m_exclusive_device_mode{ };
    std::string m_exclusive_device_uuid{ };
  public:
    using entry_point = int(const i32, const ptr<csequence>, system&);

    void enable_exclusive_device_mode(const bool enable);
    void is_exclusive_device_mode_enabled() const;
    void set_exclusive_device_uuid(const std::string& uuid);
    std::string exclusive_device_uuid() const;

    int run(const std::function<entry_point>& fn, const i32 argc, const ptr<csequence> argv);
  };

}

#endif
