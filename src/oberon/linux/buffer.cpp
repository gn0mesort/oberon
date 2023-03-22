#include "oberon/linux/buffer.hpp"

#include <cstring>

#include <algorithm>

namespace oberon::linux {

  buffer::buffer(const buffer_type type, vk_device& device, const VkDeviceSize size) :
  m_owning_device{ &device }, m_size{ size } {
    auto buffer_info = VkBufferCreateInfo{ };
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    auto allocation_info = VmaAllocationCreateInfo{ };
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    m_staging = m_owning_device->allocate_buffer(buffer_info, allocation_info);
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    switch (type)
    {
    case buffer_type::vertex:
      buffer_info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      break;
    case buffer_type::index:
      buffer_info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      break;
    default:
      break;
    }
    m_resident_usage = buffer_info.usage;
    allocation_info.flags = 0;
    m_resident = m_owning_device->allocate_buffer(buffer_info, allocation_info);
  }

  buffer::~buffer() noexcept {
    m_owning_device->free_buffer(m_staging);
    // Causes an error if the buffer is bound?
    m_owning_device->free_buffer(m_resident);
  }

  void buffer::write(const csequence data, usize sz) {
    auto dest = m_owning_device->writable_ptr(m_staging);
    auto count = std::min(sz, m_size);
    std::memcpy(dest, data, count);
    m_owning_device->flush_buffer(m_staging);
    m_owning_device->copy_buffer(m_resident->buffer, m_staging->buffer, m_size);
    if (m_resident_usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
    {
      m_owning_device->insert_buffer_memory_barrier(m_resident_usage);
    }
  }

  VkBuffer buffer::resident() {
    return m_resident->buffer;
  }

  VkDeviceSize buffer::size() const {
    return m_size;
  }

}
