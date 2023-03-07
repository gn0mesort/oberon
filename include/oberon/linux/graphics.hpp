#ifndef OBERON_LINUX_GRAPHICS_HPP
#define OBERON_LINUX_GRAPHICS_HPP

#include "../memory.hpp"
#include "../graphics.hpp"

#include "vk.hpp"

namespace oberon::linux {

  class system;
  class window;

  class graphics final : public oberon::graphics {
  private:
    struct queue_selection final {
      u32 graphics_queue{ };
      u32 presentation_queue{ };
    };

    static queue_selection select_queues_heuristic(const graphics& gfx, const u32 vendor,
                                                   const VkPhysicalDevice device);
    static queue_selection select_queues_amd(const graphics& gfx, const u32 vendor, const VkPhysicalDevice device);
    static queue_selection select_queues_nvidia(const graphics& gfx, const u32 vendor, const VkPhysicalDevice device);
    static queue_selection select_queues_intel(const graphics& gfx, const u32 vendor, const VkPhysicalDevice device);

    ptr<system> m_parent{ };
    ptr<window> m_target{ };

    std::vector<VkPhysicalDevice> m_physical_devices{ };
    std::vector<graphics_device> m_graphics_devices{ };
    queue_selection m_selected_queue_families{ };
    VkPhysicalDevice m_vk_selected_physical_device{ };
    VkDevice m_vk_device{ };
    VkQueue m_vk_graphics_queue{ };
    VkQueue m_vk_present_queue{ };

    void initialize_device(const VkPhysicalDevice physical_device);
    void deinitialize_device();
  public:
    graphics(system& sys, window& win);
    graphics(const graphics& other) = delete;
    graphics(graphics&& other) = delete;

    ~graphics() noexcept;

    graphics& operator=(const graphics& rhs) = delete;
    graphics& operator=(graphics&& rhs) = delete;

    const std::vector<graphics_device>& available_devices() const override;
    void select_device(const graphics_device& device) override;
    void wait_for_device_to_idle() override;
  };

}

#endif
