#ifndef OBERON_LINUX_VK_DEVICE_HPP
#define OBERON_LINUX_VK_DEVICE_HPP

#include <array>
#include <vector>
#include <unordered_set>
#include <string>
#include <list>

#include "../uniform_buffers.hpp"

#include "vk.hpp"


namespace oberon::linux {

namespace vk_device_status {
  OBERON_DEFINE_ZERO_BIT(none);
  OBERON_DEFINE_BIT(dirty, 1);
  OBERON_DEFINE_BIT(rendering, 2);
}

  class vk_device {
  private:
    struct buffer_allocation final {
      VkBuffer buffer{ };
      VmaAllocation allocation{ };
    };
  public:
    using buffer_iterator = typename std::list<buffer_allocation>::const_iterator;
  private:
    template <typename Type>
    using per_frame_array = std::array<Type, OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT>;

    enum {
      SEMAPHORE_IMAGE_ACQUIRED = 0,
      SEMAPHORE_RENDER_FINISHED = 1,
      SEMAPHORE_MAX
    };

    enum {
      COMMAND_BUFFER_TRANSFER = 0,
      COMMAND_BUFFER_RENDER = 1,
      COMMAND_BUFFER_MAX
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
    VmaAllocator m_allocator{ };
    per_frame_array<VkCommandPool> m_command_pools{ };
    per_frame_array<std::array<VkCommandBuffer, COMMAND_BUFFER_MAX>> m_command_buffers{ };
    per_frame_array<std::array<VkSemaphore, SEMAPHORE_MAX>> m_semaphores{ };
    per_frame_array<VkFence> m_fences{ };
    VkSurfaceFormatKHR m_surface_format{ };
    VkFormat m_depth_stencil_format{ VK_FORMAT_D16_UNORM };
    VkImageAspectFlags m_depth_stencil_aspects{ VK_IMAGE_ASPECT_DEPTH_BIT };
    VkRect2D m_render_area{ };
    VkPresentModeKHR m_present_mode{ VK_PRESENT_MODE_FIFO_KHR };
    u32 m_buffer_count{ 3 };
    VkSwapchainKHR m_swapchain{ };
    std::vector<VkImage> m_swapchain_images{ };
    std::vector<VkImage> m_depth_stencil_images{ };
    std::vector<VkImageView> m_swapchain_image_views{ };
    std::vector<VkImageView> m_depth_image_views{ };
    std::vector<VmaAllocation> m_depth_stencil_allocations{ };
    bitmask m_status{ vk_device_status::none_bit };
    u32 m_current_image{ };
    u32 m_current_frame{ };
    std::vector<VkPipelineLayout> m_interned_layouts{ };
    std::vector<VkPipeline> m_interned_graphics_pipelines{ };
    std::list<buffer_allocation> m_buffers{ };
    std::unordered_set<std::string> m_enabled_extensions{ };
    std::unordered_set<VkPresentModeKHR> m_available_present_modes{ };
    VkDescriptorSetLayout m_uniform_descriptor_layout{ };
    VkDescriptorPool m_descriptor_pool{ };
    per_frame_array<VkDescriptorSet> m_descriptor_sets{ };
    per_frame_array<buffer_iterator> m_uniform_staging{ };
    per_frame_array<buffer_iterator> m_uniform_resident{ };


    virtual void select_device_queues(const vkfl::loader& dl, const VkSurfaceKHR surface,
                                      const VkPhysicalDevice physical_device, i64& graphics_queue_families,
                                      i64& transfer_queue_family, i64& present_queue_family);
    virtual std::unordered_set<std::string> query_required_extensions() const;
    virtual std::unordered_set<std::string> query_requested_extensions() const;
    virtual void select_surface_format(const vkfl::loader& dl, const VkPhysicalDevice physical_device,
                                       const VkSurfaceKHR surface, VkSurfaceFormatKHR& surface_format);
    void create_swapchain(const VkExtent2D& desired_image_extent, const VkSwapchainKHR old);
    void destroy_swapchain(const VkSwapchainKHR swapchain) noexcept;
    void create_depth_stencil_images();
    void destroy_depth_stencil_images() noexcept;
    void create_swapchain_image_views();
    void destroy_swapchain_image_views() noexcept;
    void recreate_swapchain();
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
    void dirty();
    void request_present_mode(const VkPresentModeKHR mode);
    void request_swapchain_images(const u32 images);
    VkShaderModule create_shader_module(const VkShaderModuleCreateInfo& info);
    void destroy_shader_module(const VkShaderModule shader_module) noexcept;
    VkFormat current_swapchain_format() const;
    VkFormat current_depth_stencil_format() const;
    VkPresentModeKHR current_present_mode() const;
    u32 current_swapchain_size() const;
    VkDescriptorSetLayout uniform_descriptor_layout();
    VkPipelineLayout intern_pipeline_layout(const VkPipelineLayoutCreateInfo& info);
    VkPipeline intern_graphics_pipeline(const VkGraphicsPipelineCreateInfo& info);
    void begin_frame();
    void end_frame();
    void draw(const VkPipeline pipeline, const u32 vertices);
    void draw_buffer(const VkPipeline pipeline, const VkPipelineLayout layout, const VkBuffer buffer,
                     const u32 vertices);
    buffer_iterator allocate_buffer(const VkBufferCreateInfo& buffer_info,
                                    const VmaAllocationCreateInfo& allocation_info);
    void write_uniform_buffer(const uniform_buffer& ub);
    void free_buffer(const buffer_iterator itr);
    csequence writable_ptr(const buffer_iterator itr);
    void flush_buffer(const buffer_iterator itr);
    void copy_buffer(const VkBuffer dst, const VkBuffer src, const VkDeviceSize sz);
    void insert_buffer_memory_barrier(const VkBufferUsageFlags usage);
    void wait_device_idle() const noexcept;
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
