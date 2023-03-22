#ifndef OBERON_LINUX_BUFFER_HPP
#define OBERON_LINUX_BUFFER_HPP

#include "../buffer.hpp"

#include "vk.hpp"
#include "vk_device.hpp"

namespace oberon::linux {

  class buffer final : public oberon::buffer {
  private:
    ptr<vk_device> m_owning_device{ };

    VkBufferUsageFlags m_resident_usage{ };
    VkDeviceSize m_size{ };
    vk_device::buffer_iterator m_staging{ };
    vk_device::buffer_iterator m_resident{ };

    void write(const csequence input, const usize sz) override;
  public:
    buffer(const buffer_type type, vk_device& device, const VkDeviceSize size);
    buffer(const buffer& other) = delete;
    buffer(buffer&& other) = delete;

    ~buffer() noexcept;

    buffer& operator=(const buffer& rhs) = delete;
    buffer& operator=(buffer&& rhs) = delete;

    VkBuffer resident();
    VkDeviceSize size() const;
  };

}

#endif
