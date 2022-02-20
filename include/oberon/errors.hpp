#ifndef OBERON_ERRORS_HPP
#define OBERON_ERRORS_HPP

#include <string>
#include <string_view>

#include <exception>

#include "memory.hpp"

namespace oberon {

  class error : public std::exception {
  public:
    virtual ~error() noexcept = default;

    virtual std::u8string_view message() const noexcept = 0;
    virtual i32 result() const noexcept = 0;
  };

}

#endif
