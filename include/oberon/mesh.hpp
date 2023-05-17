#ifndef OBERON_MESH_HPP
#define OBERON_MESH_HPP

#include <ranges>
#include <iterator>

#include "memory.hpp"
#include "vertices.hpp"

#include "concepts/has_internal_implementation.hpp"

#define RANGE_SIZE(range) \
  (std::size(range) * sizeof(typename std::iterator_traits<decltype(std::begin(range))>::value_type))

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(mesh_impl);

}

namespace oberon {

  class graphics_device;

  class mesh final {
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::mesh_impl);

    mesh(graphics_device& device, const vertex_type vertex, const readonly_csequence data, const usize size);
  public:
    using implementation_type = internal::base::mesh_impl;

    template <std::ranges::contiguous_range Type>
    mesh(graphics_device& device, Type&& range) :
    mesh{ device, default_vertex_type, reinterpret_cast<readonly_csequence>(std::data(range)), RANGE_SIZE(range) } { }
    template <std::ranges::contiguous_range Type>
    mesh(graphics_device& device, const vertex_type vertex, Type&& range) :
    mesh{ device, vertex, reinterpret_cast<readonly_csequence>(std::data(range)), RANGE_SIZE(range) } { }
    mesh(const mesh& other) = delete;
    mesh(mesh&& other) = delete;

    ~mesh() noexcept = default;

    mesh& operator=(const mesh& rhs) = delete;
    mesh& operator=(mesh&& rhs) = delete;

    implementation_type& implementation();
    void rotate(const f32 radians, const glm::vec3& axis);
  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, mesh);

}

#undef RANGE_SIZE

#endif
