#include "oberon/mesh.hpp"

#include "oberon/internal/base/mesh_impl.hpp"

namespace oberon {

  mesh::mesh(graphics_device& device, const vertex_type vertex, const readonly_csequence data, const usize size) :
  m_impl{ new internal::base::mesh_impl{ device, vertex, data, size } } { }

  mesh::implementation_type& mesh::implementation() {
    return *m_impl;
  }

  void mesh::rotate(const f32 radians, const glm::vec3& axis) {
    m_impl->rotate(radians, axis);
  }


}
