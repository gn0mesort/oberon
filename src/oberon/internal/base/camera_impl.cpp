#include "oberon/internal/base/camera_impl.hpp"

#include <cstring>

#include "oberon/graphics_device.hpp"
#include "oberon/render_window.hpp"
#include "oberon/camera.hpp"

#include "oberon/internal/base/vulkan.hpp"
#include "oberon/internal/base/render_window_impl.hpp"

#define VK_STRUCT(name) \
  OBERON_INTERNAL_BASE_VK_STRUCT(name)

#define VK_DECLARE_PFN(dl, name) \
  OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, name)

#define VK_SUCCEEDS(exp) \
  OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp)

namespace oberon::internal::base {

  void camera_impl_dtor::operator()(ptr<camera_impl> p) const noexcept {
    delete p;
  }

  camera_impl::camera_impl(graphics_device& device, const glm::mat4& projection, const glm::vec3& position) :
  m_parent{ &device }, m_data{ glm::identity<glm::mat4>(), projection } {
    auto& parent = m_parent->implementation();
    auto buffer_info = VkBufferCreateInfo{ };
    buffer_info.sType = VK_STRUCT(BUFFER_CREATE_INFO);
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    auto family = parent.queue_family();
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &family;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.size = sizeof(camera_data);
    auto allocation_info = VmaAllocationCreateInfo{ };
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    m_staging = parent.create_buffer(buffer_info, allocation_info);
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocation_info.flags = 0;
    allocation_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    m_resident = parent.create_buffer(buffer_info, allocation_info);
    m_data.view = glm::lookAt(position, { 0, 0, 0 }, UP);
  }

  camera_impl::~camera_impl() noexcept {
    auto& parent = m_parent->implementation();
    parent.wait_for_idle();
    for (auto& window : m_active_windows)
    {
      window->clear_active_camera();
    }
    parent.destroy_buffer(m_resident);
    parent.destroy_buffer(m_staging);
  }

  void camera_impl::update_resident() {
    if (!m_dirty || m_active_windows.empty())
    {
      return;
    }
    std::memcpy(m_staging->info.pMappedData, &m_data, sizeof(camera_data));
    auto& parent = m_parent->implementation();
    parent.flush_buffer(m_staging);
    auto& window = *m_active_windows.back();
    window.copy_buffer(m_staging->buffer, m_resident->buffer, sizeof(camera_data));
    auto barrier = VkMemoryBarrier{ };
    barrier.sType = VK_STRUCT(MEMORY_BARRIER);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
    window.insert_memory_barrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    m_dirty = false;
  }

  camera_impl::window_iterator camera_impl::attach_window(render_window_impl& window) {
    m_active_windows.emplace_front(&window);
    update_resident();
    return m_active_windows.begin();
  }

  void camera_impl::detach_window(camera_impl::window_iterator window) {
    m_active_windows.erase(window);
  }

  VkBuffer camera_impl::resident_buffer() const {
    return m_resident->buffer;
  }

  void camera_impl::look_at(const glm::vec3& position, const glm::vec3& target) {
    m_data.view = glm::lookAt(position, target, UP);
    m_dirty = true;
    update_resident();
  }

}
