#ifndef OBERON_ERRORS_HPP
#define OBERON_ERRORS_HPP

#include <string>
#include <string_view>

#include "memory.hpp"

namespace oberon {
  class error {
  public:
    virtual ~error() noexcept = default;

    virtual cstring message() const noexcept = 0;
    virtual i32 result() const noexcept = 0;
  };

  // Errors that cannot under any circumstances be recovered from.
  class critical_error : public error {
  private:
    cstring m_message{ };
    i32 m_result{ 1 };
  public:
    critical_error(const cstring message) noexcept;
    critical_error(const cstring message, const i32 result) noexcept;
    critical_error(const critical_error& other) noexcept = default;
    critical_error(critical_error&& other) noexcept = default;

    virtual ~critical_error() noexcept = default;

    critical_error& operator=(const critical_error& rhs) noexcept = default;
    critical_error& operator=(critical_error&& rhs) noexcept = default;

    virtual cstring message() const noexcept override;
    virtual i32 result() const noexcept override;
  };

  // Errors that should likely cause termination.
  class fatal_error : public error {
  private:
    std::string m_message{ };
    i32 m_result{ 1 };
  public:
    fatal_error(const std::string_view& message);
    fatal_error(const std::string_view& message, const i32 result);
    fatal_error(const fatal_error& other) = default;
    fatal_error(fatal_error&& other) = default;

    virtual ~fatal_error() noexcept = default;

    fatal_error& operator=(const fatal_error& rhs) = default;
    fatal_error& operator=(fatal_error&& rhs) = default;

    virtual cstring message() const noexcept override;
    virtual i32 result() const noexcept override;
  };


  // Erros that should not cause termination.
  class nonfatal_error : public error {
  private:
    std::string m_message{ };
    i32 m_result{ 0 };
  public:
    nonfatal_error(const std::string_view& message);
    nonfatal_error(const std::string_view& message, const i32 result);
    nonfatal_error(const nonfatal_error& other) = default;
    nonfatal_error(nonfatal_error&& other) = default;

    virtual ~nonfatal_error() noexcept = default;

    nonfatal_error& operator=(const nonfatal_error& rhs) = default;
    nonfatal_error& operator=(nonfatal_error&& rhs) = default;

    virtual cstring message() const noexcept override;
    virtual i32 result() const noexcept override;
  };
}

#endif
