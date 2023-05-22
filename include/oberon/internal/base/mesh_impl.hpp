/**
 * @file mesh_impl.hpp
 * @brief Internal mesh API.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
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

  /**
   * @class mesh_impl
   * @brief The base implementation of a 3D mesh.
   */
  class mesh_impl final {
  private:
    glm::mat4 m_transform{ glm::identity<glm::mat4>() };
    ptr<graphics_device> m_parent{ };
    vertex_type m_vertex_type{ };
    usize m_vertex_size{ };
    usize m_size{ };
    graphics_device_impl::buffer_iterator m_staging{ };
    graphics_device_impl::buffer_iterator m_resident{ };
    bool m_dirty{ true };
  public:
    /**
     * @brief Create a `mesh_impl`.
     * @param device The `graphics_device` on which the mesh will be based.
     * @param vertex The `vertex_type` of the mesh.
     * @param data A sequence of bytes representing the mesh.
     * @param size The number of bytes in the mesh.
     */
    mesh_impl(graphics_device& device, const vertex_type vertex, const readonly_csequence data, const usize size);

    /// @cond
    mesh_impl(const mesh_impl& other) = delete;
    mesh_impl(mesh_impl&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `mesh_impl`.
     */
    ~mesh_impl() noexcept;

    /// @cond
    mesh_impl& operator=(const mesh_impl& rhs) = delete;
    mesh_impl& operator=(mesh_impl&& rhs) = delete;
    /// @endcond


    /**
     * @brief Retrieve the `vertex_type` of the mesh.
     * @return The `vertex_type`.
     */
    vertex_type type() const;

    /**
     * @brief Retrieve the size of one vertex in the mesh.
     * @return The size of the current `vertex_type` in bytes.
     */
    usize vertex_size() const;
    /**
     * @brief Retrieve the number of vertices in the mesh.
     * @return The number of vertices in the mesh.
     */
    usize size() const;

    /**
     * @brief Retrieve the current transformation of the matrix.
     * @return A 4x4 matrix representing the mesh's transform.
     */
    const glm::mat4& transform() const;

    /**
     * @brief Retrieve the staging `VkBuffer` belonging to the mesh.
     * @return The staging `VkBuffer`.
     */
    VkBuffer staging_buffer();

    /**
     * @brief Retrieve the resident `VkBuffer` belonging to the mesh.
     * @return The resident `VkBuffer`.
     */
    VkBuffer resident_buffer();

    /**
     * @brief Determine if the mesh needs to be updated on the underlying device or not.
     * @return True if the mesh has changed since it was last transfered. False in all other cases.
     */
    bool is_dirty() const;

    /**
     * @brief Mark a dirty mesh as clean.
     */
    void clean();

    /**
     * @brief Rotate the mesh about an axis.
     * @param radians The magnitude of the rotation in radians.
     * @param axis A normalized 3D vector representing the axes to rotate about.
     */
    void rotate(const f32 radians, const glm::vec3& axis);
  };

}

#endif
