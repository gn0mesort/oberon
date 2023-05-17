#ifndef OBERON_CAMERA_HPP
#define OBERON_CAMERA_HPP

#include "memory.hpp"
#include "glm.hpp"

#include "concepts/has_internal_implementation.hpp"

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(camera_impl);

}

namespace oberon {

  class graphics_device;

  class camera final {
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::camera_impl);
  public:
    using implementation_type = internal::base::camera_impl;

    camera(graphics_device& device, const glm::mat4& projection, const glm::vec3& position);
    camera(const camera& other) = delete;
    camera(camera&& other) = delete;

    ~camera() noexcept = default;

    camera& operator=(const camera& rhs) = delete;
    camera& operator=(camera&& rhs) = delete;

    void look_at(const glm::vec3& position, const glm::vec3& target);

    implementation_type& implementation();
  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, camera);

}

#endif
