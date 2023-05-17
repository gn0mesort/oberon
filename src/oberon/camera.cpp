#include "oberon/camera.hpp"

#include "oberon/internal/base/camera_impl.hpp"

namespace oberon {

  camera::camera(graphics_device& device, const glm::mat4& projection, const glm::vec3& position) :
  m_impl{ new internal::base::camera_impl{ device, projection, position } } { }

  void camera::look_at(const glm::vec3& position, const glm::vec3& target) {
    m_impl->look_at(position, target);
  }

  camera::implementation_type& camera::implementation() {
    return *m_impl;
  }

}
