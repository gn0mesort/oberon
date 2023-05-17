#ifndef OBERON_INTERNAL_BASE_CAMERA_IMPL_HPP
#define OBERON_INTERNAL_BASE_CAMERA_IMPL_HPP

#include <list>

#include "../../glm.hpp"

#include "graphics_device_impl.hpp"

namespace oberon {

  class graphics_device;

}

namespace oberon::internal::base {

  class render_window_impl;

  class camera_impl final {
  private:
    static constexpr const glm::vec3 UP{ 0.0, 1.0, 0.0 };

    struct camera_data final {
      glm::mat4 view{ glm::identity<glm::mat4>() };
      glm::mat4 projection{ };
    };

    ptr<graphics_device> m_parent{ };
    std::list<ptr<render_window_impl>> m_active_windows{ };
    graphics_device_impl::buffer_iterator m_staging{ };
    graphics_device_impl::buffer_iterator m_resident{ };
    bool m_dirty{ true };
    camera_data m_data{ };

    void update_resident();
  public:
    using window_iterator = std::list<ptr<render_window_impl>>::iterator;

    camera_impl(graphics_device& device, const glm::mat4& projection, const glm::vec3& position);
    camera_impl(const camera_impl& other) = delete;
    camera_impl(camera_impl&& other) = delete;

    ~camera_impl() noexcept;

    camera_impl& operator=(const camera_impl& rhs) = delete;
    camera_impl& operator=(camera_impl&& rhs) = delete;

    window_iterator attach_window(render_window_impl& window);
    void detach_window(window_iterator window);
    VkBuffer resident_buffer() const;
    void look_at(const glm::vec3& position, const glm::vec3& target);

  };

}

#endif
