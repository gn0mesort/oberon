#ifndef OBERON_ERRORS_HPP
#define OBERON_ERRORS_HPP

#include <string>
#include <string_view>
#if __has_include(<source_location>) && !defined(__clang__)
  #include <source_location>
#elif __has_include(<experimental/source_location>)
  #include <experimental/source_location>

  namespace std {

    using source_location = std::experimental::source_location;

  }
#else
  #error This header requires std::source_location.
#endif
#include <exception>

#include "types.hpp"
#include "memory.hpp"

namespace oberon {
inline namespace errors_v01 {

  using iresult = imax;

  class error : public std::exception {
  public:
    virtual ~error() noexcept = default;

    virtual iresult result() const noexcept = 0;
    virtual std::string_view message() const noexcept = 0;
    virtual const std::source_location& location() const noexcept = 0;
  };

  class simple_error : public error {
  private:
    std::string m_message{ };
    std::source_location m_location{ };
  public:
    simple_error(const cstring msg, const std::source_location& loc = std::source_location::current());
    simple_error(const std::string& msg, const std::source_location& loc = std::source_location::current());

    virtual ~simple_error() noexcept = default;

    cstring what() const noexcept override;
    std::string_view message() const noexcept override;
    iresult result() const noexcept override;
    const std::source_location& location() const noexcept override;
  };

}
}

#endif
