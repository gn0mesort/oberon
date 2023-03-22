#ifndef OBERON_BUFFER_HPP
#define OBERON_BUFFER_HPP

#include <ranges>
#include <iterator>

#include "memory.hpp"

namespace oberon {

  enum class buffer_type {
    none = 0,
    vertex,
    index
  };

  class buffer {
  protected:
    virtual void write(const csequence input, const usize sz) = 0;
  public:
    buffer() = default;
    buffer(const buffer& other) = default;
    buffer(buffer&& other) = default;

    inline virtual ~buffer() noexcept = 0;

    buffer& operator=(const buffer& rhs) = default;
    buffer& operator=(buffer&& rhs) = default;

    template <std::ranges::contiguous_range Range>
    void write(Range&& input);
  };

  buffer::~buffer() noexcept { }

  template <std::ranges::contiguous_range Range>
  void buffer::write(Range&& input) {
    using value_type = typename std::iterator_traits<decltype(std::ranges::begin(input))>::value_type;
    write(reinterpret_cast<csequence>(std::ranges::data(input)), std::ranges::size(input) * sizeof(value_type));
  }

}

#endif
