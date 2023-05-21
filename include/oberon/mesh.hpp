/**
 * @file mesh.hpp
 * @brief 3D mesh objects.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_MESH_HPP
#define OBERON_MESH_HPP

#include <ranges>
#include <iterator>

#include "memory.hpp"
#include "vertices.hpp"

#include "concepts/has_internal_implementation.hpp"

/// @cond
#define RANGE_SIZE(range) \
  (std::size(range) * sizeof(typename std::iterator_traits<decltype(std::begin(range))>::value_type))
/// @endcond

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(mesh_impl);

}

namespace oberon {

  class graphics_device;

  /**
   * @class mesh
   * @brief An object representing a 3D mesh that can be rendered using a `renderer`.
   * @details It is undefined behavior to use a `mesh` object created from a specific `graphics_device` with objects
   *          created by other `graphics_device`s.
   */
  class mesh final {
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::mesh_impl);

    mesh(graphics_device& device, const vertex_type vertex, const readonly_csequence data, const usize size);
  public:
    using implementation_type = internal::base::mesh_impl;

    /**
     * @brief Create a `mesh` using the specified `graphics_device` and vertex data.
     * @details Vertex data will be interpreted using the library's default vertex type.
     * @tparam Type A type representing a contiguous range of bytes to interpret vertex data from.
     * @param device The `graphics_device` that the new `mesh` will belong to.
     * @param range A contiguous range of bytes to interpret as vertex data.
     */
    template <std::ranges::contiguous_range Type>
    mesh(graphics_device& device, Type&& range) :
    mesh{ device, default_vertex_type, reinterpret_cast<readonly_csequence>(std::data(range)), RANGE_SIZE(range) } { }

    /**
     * @brief Create a `mesh` using the specified `graphics_device` and vertex data.
     * @details Vertex data will be interpreted using the `vertex_type` specified by `vertex`.
     * @tparam Type A type representing a contiguous range of bytes to interpret vertex data from.
     * @param device The `graphics_device` that the new `mesh` will belong to.
     * @param vertex The type of vertex data being provided.
     * @param range A contiguous range of bytes to interpret as vertex data.
     */
    template <std::ranges::contiguous_range Type>
    mesh(graphics_device& device, const vertex_type vertex, Type&& range) :
    mesh{ device, vertex, reinterpret_cast<readonly_csequence>(std::data(range)), RANGE_SIZE(range) } { }

    /// @cond
    mesh(const mesh& other) = delete;
    mesh(mesh&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `mesh`.
     */
    ~mesh() noexcept = default;

    /// @cond
    mesh& operator=(const mesh& rhs) = delete;
    mesh& operator=(mesh&& rhs) = delete;
    /// @endcond


    /**
     * @brief Retrieve the `mesh`'s implementation.
     * @return A reference to the `mesh`'s implementation object.
     */
    implementation_type& implementation();

    /**
     * @brief Rotate the `mesh` about an axis by the specified number of radians.
     * @param radians The magnitude of rotation measured in radians.
     * @param axis The normalized set of axes to rotate the `mesh` about.
     */
    void rotate(const f32 radians, const glm::vec3& axis);
  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, mesh);

}

#undef RANGE_SIZE

#endif
