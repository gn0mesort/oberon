#ifndef OBERON_CAMERA_HPP
#define OBERON_CAMERA_HPP

#include "memory.hpp"
#include "glm.hpp"

namespace oberon {


  class camera final {
  private:
    glm::mat4 m_view{ glm::identity<glm::mat4>() };
    glm::mat4 m_projection{ };
  public:
    camera(const glm::mat4& projection);
    camera(const camera& other) = default;
    camera(camera&& other) = default;

    ~camera() noexcept = default;

    camera& operator=(const camera& rhs) = default;
    camera& operator=(camera&& rhs) = default;

    void look_at(const glm::vec3& position, const glm::vec3& target);
  };

}

#endif
