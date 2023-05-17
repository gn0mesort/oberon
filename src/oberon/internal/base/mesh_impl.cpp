#include "oberon/internal/base/mesh_impl.hpp"

#include <cstring>

#include "oberon/graphics_device.hpp"
#include "oberon/mesh.hpp"

#include "oberon/internal/base/vulkan.hpp"
#include "oberon/internal/base/render_window_impl.hpp"

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
    buffer_info.size = sizeof(mesh_data);
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    m_uniform_staging = parent.create_buffer(buffer_info, allocation_info);
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocation_info.flags = 0;
    m_uniform_resident = parent.create_buffer(buffer_info, allocation_info);
    std::memcpy(m_staging->info.pMappedData, data, size);
    parent.flush_buffer(m_staging);
  }

  mesh_impl::~mesh_impl() noexcept {
    auto& parent = m_parent->implementation();
    parent.wait_for_idle();
    parent.destroy_buffer(m_uniform_resident);
    parent.destroy_buffer(m_uniform_staging);
    parent.destroy_buffer(m_resident);
    parent.destroy_buffer(m_staging);
  }


  vertex_type mesh_impl::type() const {
    return m_vertex_type;
  }

  void mesh_impl::rotate(const f32 radians, const glm::vec3& axis) {
    m_data.transform = glm::rotate(m_data.transform, radians, axis);
    std::memcpy(m_uniform_staging->info.pMappedData, &m_data, sizeof(mesh_data));
    m_parent->implementation().flush_buffer(m_uniform_staging);
    m_status |= UNIFORM_DIRTY;
  }

  usize mesh_impl::size() const {
    return m_size;
  }

  void mesh_impl::flush_to_device(render_window_impl& win) {
    auto barrier = VkMemoryBarrier{ };
    barrier.sType = VK_STRUCT(MEMORY_BARRIER);
    if (UNIFORM_DIRTY)
    {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
      win.copy_buffer(m_uniform_staging->buffer, m_uniform_resident->buffer, sizeof(mesh_data));
      win.insert_memory_barrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    }
    if (VERTEX_DIRTY)
    {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
      win.copy_buffer(m_staging->buffer, m_resident->buffer, m_size * m_vertex_size);
      win.insert_memory_barrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
    }
    m_status &= ~(VERTEX_DIRTY | UNIFORM_DIRTY);
  }

  VkBuffer mesh_impl::uniform_resident_buffer() {
    return m_uniform_resident->buffer;
  }

  VkBuffer mesh_impl::resident_buffer() {
    return m_resident->buffer;
  }

}

