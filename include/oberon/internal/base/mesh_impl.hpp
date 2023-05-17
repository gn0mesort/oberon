#ifndef OBERON_INTERNAL_BASE_MESH_IMPL_HPP
#define OBERON_INTERNAL_BASE_MESH_IMPL_HPP

#include "../../types.hpp"
#include "../../memory.hpp"
#include "../../vertices.hpp"
#include "../../glm.hpp"

#include "graphics_device_impl.hpp"

namespace oberon {

  class graphics_device;

}

namespace oberon::internal::base {

  class render_window_impl;

  class mesh_impl final {
  private:
    static constexpr const bitmask VERTEX_DIRTY{ 1 };
    static constexpr const bitmask UNIFORM_DIRTY{ 2 };

    struct mesh_data final {
      glm::mat4 transform{ glm::identity<glm::mat4>() };
    };

    ptr<graphics_device> m_parent{ };
    vertex_type m_vertex_type{ };
    usize m_vertex_size{ };
    usize m_size{ };
    graphics_device_impl::buffer_iterator m_uniform_staging{ };
    graphics_device_impl::buffer_iterator m_uniform_resident{ };
    graphics_device_impl::buffer_iterator m_staging{ };
    graphics_device_impl::buffer_iterator m_resident{ };
    mesh_data m_data{ };
    bitmask m_status{ UNIFORM_DIRTY | VERTEX_DIRTY };
  public:
    mesh_impl(graphics_device& device, const vertex_type vertex, const readonly_csequence data, const usize size);
    mesh_impl(const mesh_impl& other) = delete;
    mesh_impl(mesh_impl&& other) = delete;

    ~mesh_impl() noexcept;

    mesh_impl& operator=(const mesh_impl& rhs) = delete;
    mesh_impl& operator=(mesh_impl&& rhs) = delete;

    vertex_type type() const;
    usize vertex_size() const;
    usize size() const;
    void flush_to_device(render_window_impl& win);
    VkBuffer uniform_resident_buffer();
    VkBuffer resident_buffer();
    void rotate(const f32 radians, const glm::vec3& axis);
  };

}

#endif
