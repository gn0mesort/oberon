#include "oberon/internal/base/mesh_impl.hpp"

#include <cstring>

#include "oberon/graphics_device.hpp"
#include "oberon/mesh.hpp"

#include "oberon/internal/base/vulkan.hpp"

#define VK_STRUCT(name) \
  OBERON_INTERNAL_BASE_VK_STRUCT(name)

#define VK_DECLARE_PFN(dl, name) \
  OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, name)

#define VK_SUCCEEDS(exp) \
  OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp)

namespace oberon::internal::base {

  void mesh_impl_dtor::operator()(ptr<mesh_impl> p) const noexcept {
    delete p;
  }

  mesh_impl::mesh_impl(graphics_device& device, const vertex_type vertex, const readonly_csequence data,
                       const usize size) :
  m_parent{ &device }, m_vertex_type{ vertex } {
    switch (m_vertex_type)
    {
    case vertex_type::position_color:
      m_vertex_size = sizeof(vertex_pc);
      break;
    case vertex_type::position_normal_texture_coordinates:
      m_vertex_size = sizeof(vertex_pnt);
      break;
    default:
      break;
    }
    m_size = size / m_vertex_size;
    auto& parent = m_parent->implementation();
    auto buffer_info = VkBufferCreateInfo{ };
    buffer_info.sType = VK_STRUCT(BUFFER_CREATE_INFO);
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 1;
    auto family = parent.queue_family();
    buffer_info.pQueueFamilyIndices = &family;
    auto allocation_info = VmaAllocationCreateInfo{ };
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    m_staging = parent.create_buffer(buffer_info, allocation_info);
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocation_info.flags = 0;
    m_resident = parent.create_buffer(buffer_info, allocation_info);
    std::memcpy(m_staging->info.pMappedData, data, size);
    VK_SUCCEEDS(vmaFlushAllocation(parent.allocator(), m_staging->allocation, 0, VK_WHOLE_SIZE));
  }

  mesh_impl::~mesh_impl() noexcept {
    auto& parent = m_parent->implementation();
    parent.wait_for_idle();
    parent.destroy_buffer(m_resident);
    parent.destroy_buffer(m_staging);
  }


  vertex_type mesh_impl::type() const {
    return m_vertex_type;
  }

  void mesh_impl::rotate(const f32 radians, const glm::vec3& axis) {
    m_transform = glm::rotate(m_transform, radians, axis);
  }

  usize mesh_impl::size() const {
    return m_size;
  }

  usize mesh_impl::vertex_size() const {
    return m_vertex_size;
  }

  bool mesh_impl::is_dirty() const {
    return m_dirty;
  }

  void mesh_impl::clean() {
    m_dirty = false;
  }

  VkBuffer mesh_impl::staging_buffer() {
    return m_staging->buffer;
  }

  VkBuffer mesh_impl::resident_buffer() {
    return m_resident->buffer;
  }

  const glm::mat4& mesh_impl::transform() const {
    return m_transform;
  }
}

