/**
 * @file camera.cpp
 * @brief 3D camera object implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/camera.hpp"

namespace oberon {

  camera::camera(const glm::mat4& projection) :
  m_projection{ projection } {
    look_at({ 0, 0, 0 }, { 0, 0, 1 });
  }

  void camera::look_at(const glm::vec3& position, const glm::vec3& target) {
    m_view = glm::lookAt(position, target, { 0, 1, 0 });
  }

}
