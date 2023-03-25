#ifndef OBERON_UNIFORM_BUFFERS_HPP
#define OBERON_UNIFORM_BUFFERS_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace oberon {

  using namespace glm;

  struct alignas(16) uniform_buffer final {
    mat4 model{ };
    mat4 view{ };
    mat4 projection{ };
  };

}

#endif
