#ifndef OBERON_LINUX_VK_DEVICE_HPP
#define OBERON_LINUX_VK_DEVICE_HPP

#include <array>
#include <vector>
#include <unordered_set>
#include <string>

#include "vk.hpp"

namespace oberon::linux {

namespace vk_device_status {
  OBERON_DEFINE_ZERO_BIT(none);
  OBERON_DEFINE_BIT(dirty, 1);
  OBERON_DEFINE_BIT(rendering, 2);
}

  class vk_device {
  private:
    template <typename Type>
    using per_frame_array = std::array<Type, OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT>;

    enum {
      SEMAPHORE_IMAGE_ACQUIRED = 0,
      SEMAPHORE_RENDER_FINISHED = 1,
      SEMAPHORE_MAX
    };

    VkSurfaceKHR m_parent_surface{ };
    VkPhysicalDevice m_parent_physical_device{ };
    vkfl::loader m_dl{ vkGetInstanceProcAddr };
    i64 m_graphics_queue_family{ -1 };
    i64 m_transfer_queue_family{ -1 };
    i64 m_present_queue_family{ -1 };
    VkDevice m_device{ };
    VkQueue m_graphics_queue{ };
    VkQueue m_transfer_queue{ };
    VkQueue m_present_queue{ };
    VkPipelineCache m_pipeline_cache{ };
    per_frame_array<VkCommandPool> m_command_pools{ };
    per_frame_array<VkCommandBuffer> m_command_buffers{ };
    per_frame_array<std::array<VkSemaphore, SEMAPHORE_MAX>> m_semaphores{ };
    per_frame_array<VkFence> m_fences{ };
    VkSurfaceFormatKHR m_surface_format{ };
    VkRect2D m_render_area{ };
    VkPresentModeKHR m_present_mode{ VK_PRESENT_MODE_FIFO_KHR };
    u32 m_buffer_count{ 3 };
    VkSwapchainKHR m_swapchain{ };
    std::vector<VkImage> m_swapchain_images{ };
    std::vector<VkImageView> m_swapchain_image_views{ };
    bitmask m_status{ vk_device_status::none_bit };
    u32 m_current_image{ };
    u32 m_current_frame{ };
    std::vector<VkPipelineLayout> m_interned_layouts{ };
    std::vector<VkPipeline> m_interned_graphics_pipelines{ };

    std::unordered_set<std::string> m_enabled_extensions{ };
    std::unordered_set<VkPresentModeKHR> m_available_present_modes{ };

    virtual void select_device_queues(const vkfl::loader& dl, const VkSurfaceKHR surface,
                                      const VkPhysicalDevice physical_device, i64& graphics_queue_families,
                                      i64& transfer_queue_family, i64& present_queue_family);
    virtual std::unordered_set<std::string> query_required_extensions() const;
    virtual std::unordered_set<std::string> query_requested_extensions() const;
    virtual void select_surface_format(const vkfl::loader& dl, const VkPhysicalDevice physical_device,
                                       const VkSurfaceKHR surface, VkSurfaceFormatKHR& surface_format);
    void create_swapchain(const VkExtent2D& desired_image_extent, const VkSwapchainKHR old);
    void destroy_swapchain(const VkSwapchainKHR swapchain) noexcept;
    void create_swapchain_image_views();
    void destroy_swapchain_image_views() noexcept;
  public:
    static std::unordered_set<std::string> query_available_extensions(const vkfl::loader& dl,
                                                                      const VkPhysicalDevice physical_device);

    vk_device(const vkfl::loader& dl, const VkSurfaceKHR surface, const VkPhysicalDevice physical_device,
              const VkExtent2D& desired_image_extent);
    vk_device(const vk_device& other) = delete;
    vk_device(vk_device&& other) = delete;

    virtual ~vk_device() noexcept;

    vk_device& operator=(const vk_device& rhs) = delete;
    vk_device& operator=(vk_device&& rhs) = delete;

    const std::unordered_set<VkPresentModeKHR>& available_present_modes() const;
    void recreate_swapchain(const u32 buffers, const VkPresentModeKHR present_mode);
    VkShaderModule create_shader_module(const VkShaderModuleCreateInfo& info);
    void destroy_shader_module(const VkShaderModule shader_module) noexcept;
    VkPipelineLayout intern_pipeline_layout(const VkPipelineLayoutCreateInfo& info);
    VkPipeline intern_graphics_pipeline(const VkGraphicsPipelineCreateInfo& info);
    void begin_frame();
    void end_frame();
    void draw(const VkPipeline pipeline, const u32 vertices);
  };

  class amd_vk_device final : public vk_device {
  private:
    void select_device_queues(const vkfl::loader& dl, const VkSurfaceKHR surface,
                              const VkPhysicalDevice physical_device, i64& graphics_queue_families,
                              i64& transfer_queue_family, i64& present_queue_family) override;
  public:
    amd_vk_device(const vkfl::loader& dl, const VkSurfaceKHR surface, const VkPhysicalDevice physical_device,
                  const VkExtent2D& desired_image_extent);
  };

  class nvidia_vk_device final : public vk_device {
  private:
    void select_device_queues(const vkfl::loader& dl, const VkSurfaceKHR surface,
                              const VkPhysicalDevice physical_device, i64& graphics_queue_families,
                              i64& transfer_queue_family, i64& present_queue_family) override;
  public:
    nvidia_vk_device(const vkfl::loader& dl, const VkSurfaceKHR surface, const VkPhysicalDevice physical_device,
                     const VkExtent2D& desired_image_extent);
  };


  class intel_vk_device final : public vk_device {
  private:
    void select_device_queues(const vkfl::loader& dl, const VkSurfaceKHR surface,
                              const VkPhysicalDevice physical_device, i64& graphics_queue_families,
                              i64& transfer_queue_family, i64& present_queue_family) override;
  public:
    intel_vk_device(const vkfl::loader& dl, const VkSurfaceKHR surface, const VkPhysicalDevice physical_device,
                    const VkExtent2D& desired_image_extent);
  };

}

#endif
