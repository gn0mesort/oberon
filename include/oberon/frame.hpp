#ifndef OBERON_FRAME_HPP
#define OBERON_FRAME_HPP

#include "memory.hpp"
#include "glm.hpp"

#include "concepts/has_internal_implementation.hpp"

namespace oberon::internal::base {

  class frame_impl;
  struct frame_info;

}

namespace oberon {

namespace image_aspect {

  OBERON_DEFINE_ZERO_BIT(none);
  OBERON_DEFINE_BIT(color, 0);
  OBERON_DEFINE_BIT(depth, 1);
  OBERON_DEFINE_BIT(stencil, 2);

}

  class camera;
  class mesh;

  class frame final {
  private:
    ptr<internal::base::frame_impl> m_impl{ };
    ptr<internal::base::frame_info> m_info{ };
  public:
    using implementation_type = internal::base::frame_impl;
    using information_type = internal::base::frame_info;

    frame(internal::base::frame_impl& impl, internal::base::frame_info& info);
    frame(const frame& other) = delete;
    frame(frame&& other) = default;

    ~frame() noexcept = default;

    frame& operator=(const frame& rhs) = delete;
    frame& operator=(frame&& rhs) = default;

    implementation_type& implementation();
    information_type& information();
    bool is_retired() const;
    void draw_test_image();
    void draw(camera& c, mesh& m);
  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, frame);

}

#endif
