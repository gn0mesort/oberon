#include "oberon/linux/vk_device.hpp"

#include <limits>
#include <algorithm>

#define DECLARE_VK_PFN(loader, command) OBERON_LINUX_VK_DECLARE_PFN(loader, command)
#define VK_SUCCEEDS(exp) OBERON_LINUX_VK_SUCCEEDS(exp)

namespace oberon::linux {

  std::unordered_set<std::string> vk_device::query_available_extensions(const vkfl::loader& dl,
                                                                        const VkPhysicalDevice physical_device) {
    auto sz = u32{ 0 };
    DECLARE_VK_PFN(dl, vkEnumerateDeviceExtensionProperties);
    VK_SUCCEEDS(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &sz, nullptr));
    auto extension_properties = std::vector<VkExtensionProperties>(sz);
    VK_SUCCEEDS(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &sz, extension_properties.data()));
    auto result = std::unordered_set<std::string>{ };
    for (const auto& extension_property : extension_properties)
    {
      result.insert(extension_property.extensionName);
    }
    return result;
  }

  void vk_device::select_device_queues(const vkfl::loader& dl, const VkSurfaceKHR surface,
                                       const VkPhysicalDevice physical_device, i64& graphics_queue_family,
                                       i64& transfer_queue_family, i64& present_queue_family) {
    // Guesstimate queues.
    DECLARE_VK_PFN(dl, vkGetPhysicalDeviceQueueFamilyProperties);
    DECLARE_VK_PFN(dl, vkGetPhysicalDeviceSurfaceSupportKHR);
    auto sz = u32{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, nullptr);
    auto queue_family_properties = std::vector<VkQueueFamilyProperties>(sz);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, queue_family_properties.data());
    auto surface_support = VkBool32{ };
#define MISSING_GRAPHICS (m_graphics_queue_family == -1)
#define MISSING_TRANSFER (m_transfer_queue_family == -1)
#define MISSING_PRESENT (m_present_queue_family == -1)
#define MISSING_QUEUES (MISSING_GRAPHICS || MISSING_TRANSFER || MISSING_PRESENT)
#define TRANSFER_CAPABLE_QUEUE (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)
    for (auto i = usize{ 0 }; i < queue_family_properties.size() && MISSING_QUEUES; ++i)
    {
      if (MISSING_GRAPHICS && queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        graphics_queue_family = i;
      }
      if (MISSING_TRANSFER && queue_family_properties[i].queueFlags & TRANSFER_CAPABLE_QUEUE)
      {
        transfer_queue_family = i;
      }
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceSupportKHR(m_parent_physical_device, i, surface, &surface_support));
      if (MISSING_PRESENT && surface_support)
      {
        present_queue_family = i;
      }
    }
#undef TRANSFER_CAPABLE_QUEUE
#undef MISSING_QUEUES
#undef MISSING_PRESENT
#undef MISSING_TRANSFER
#undef MISSING_GRAPHICS
  }

  void vk_device::select_surface_format(const vkfl::loader& dl, const VkPhysicalDevice physical_device,
                                        const VkSurfaceKHR surface, VkSurfaceFormatKHR& surface_format) {
    DECLARE_VK_PFN(dl, vkGetPhysicalDeviceSurfaceFormatsKHR);
    auto sz = u32{ 0 };
    VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &sz, nullptr));
    auto surface_formats = std::vector<VkSurfaceFormatKHR>(sz);
    VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &sz, surface_formats.data()));
    auto found_bgra = false;
    auto found_rgba = false;
    // B8G8R8A8 with SRGB is available on about 46% of reported devices on gpuinfo.org.
    // R8G8B8A8 with SRGB is available on about 62% of reported devices on gpuinfo.org
    // https://vulkan.gpuinfo.org/listsurfaceformats.php
    for (auto cur = surface_formats.cbegin(); cur != surface_formats.cend(); ++cur)
    {
      found_bgra = found_bgra ||
                   (cur->format == VK_FORMAT_B8G8R8A8_SRGB && cur->colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR);
      found_rgba = found_rgba ||
                   (cur->format == VK_FORMAT_R8G8B8A8_SRGB && cur->colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR);
    }
    // If one of the desired formats is found then select it.
    if (found_bgra)
    {
      surface_format = { VK_FORMAT_B8G8R8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    }
    else if (found_rgba)
    {
      surface_format = { VK_FORMAT_R8G8B8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    }
    // Otherwise just use the first format.
    else
    {
      surface_format = surface_formats.front();
    }
  }

  std::unordered_set<std::string> vk_device::query_required_extensions() const {
    return { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  }

  std::unordered_set<std::string> vk_device::query_requested_extensions() const {
    return { };
  }

  vk_device::vk_device(const vkfl::loader& dl, VkSurfaceKHR surface, const VkPhysicalDevice physical_device,
                       const VkExtent2D& desired_image_extent) :
  m_parent_surface{ surface }, m_parent_physical_device{ physical_device }, m_dl{ dl } {
    auto device_info = VkDeviceCreateInfo{ };
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // Collect enabled features.
    {
      DECLARE_VK_PFN(m_dl, vkGetPhysicalDeviceFeatures2);
      auto physical_device_features = VkPhysicalDeviceFeatures2{ };
      physical_device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
      auto physical_device_features_1_1 = VkPhysicalDeviceVulkan11Features{ };
      physical_device_features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
      auto physical_device_features_1_2 = VkPhysicalDeviceVulkan12Features{ };
      physical_device_features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
      auto physical_device_features_1_3 = VkPhysicalDeviceVulkan13Features{ };
      physical_device_features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
      physical_device_features.pNext = &physical_device_features_1_1;
      physical_device_features_1_1.pNext = &physical_device_features_1_2;
      physical_device_features_1_2.pNext = &physical_device_features_1_3;
      vkGetPhysicalDeviceFeatures2(m_parent_physical_device, &physical_device_features);
      device_info.pNext = &physical_device_features;
      device_info.pEnabledFeatures = nullptr;
    }
    // Select queues.
    select_device_queues(m_dl, m_parent_surface, m_parent_physical_device, m_graphics_queue_family,
                         m_transfer_queue_family, m_present_queue_family);
    auto unique_queue_families = std::unordered_set<i64>{ m_graphics_queue_family, m_transfer_queue_family,
                                                          m_present_queue_family };
    OBERON_CHECK_ERROR_MSG(unique_queue_families.size() == 1, 1, "Multiqueue support is incomplete.");
    // Prepare queue creation info.
    auto queue_priority = 1.0f;
    auto queue_infos = std::vector<VkDeviceQueueCreateInfo>(unique_queue_families.size());
    {
      auto cur = queue_infos.begin();
      for (const auto queue_family : unique_queue_families)
      {
        cur->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        cur->queueCount = 1;
        cur->queueFamilyIndex = queue_family;
        cur->pQueuePriorities = &queue_priority;
      }
    }
    device_info.pQueueCreateInfos = queue_infos.data();
    device_info.queueCreateInfoCount = queue_infos.size();
    // Select device extensions
    auto available_extensions = query_available_extensions(m_dl, m_parent_physical_device);
    auto required_extensions = query_required_extensions();
    auto requested_extensions = query_requested_extensions();
    auto selected_extensions = std::vector<cstring>{ };
    {
#define REQUIRE_VK_EXTENSION_STR(ext) \
  do \
  { \
    if (!available_extensions.contains((ext))) \
    { \
      throw vk_error { "The selected Vulkan device does not support \"" + (ext) + "\"", 1 }; \
    } \
    selected_extensions.emplace_back((ext).c_str()); \
    m_enabled_extensions.insert((ext)); \
  } \
  while (0)
#define REQUEST_VK_EXTENSION_STR(ext) \
  do \
  { \
    if (available_extensions.contains((ext))) \
    { \
      selected_extensions.emplace_back((ext).c_str()); \
      m_enabled_extensions.insert((ext)); \
    } \
  } \
  while (0)
      // This has to be true according to the specification
      // see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceCreateInfo.html
      available_extensions.erase(VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME);
      if (available_extensions.contains(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
      {
        available_extensions.erase(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
      }
      for (const auto& required_extension : required_extensions)
      {
        REQUIRE_VK_EXTENSION_STR(required_extension);
      }
      for (const auto& requested_extension : requested_extensions)
      {
        REQUEST_VK_EXTENSION_STR(requested_extension);
      }
#undef REQUEST_VK_EXTENSION
#undef REQUIRE_VK_EXTENSION
    }
    device_info.ppEnabledExtensionNames = selected_extensions.data();
    device_info.enabledExtensionCount = selected_extensions.size();
    DECLARE_VK_PFN(m_dl, vkGetPhysicalDeviceFeatures2);
    auto features = VkPhysicalDeviceFeatures2{ };
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    auto features_1_1 = VkPhysicalDeviceVulkan11Features{ };
    features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    auto features_1_2 = VkPhysicalDeviceVulkan12Features{ };
    features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    auto features_1_3 = VkPhysicalDeviceVulkan13Features{ };
    features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features.pNext = &features_1_1;
    features_1_1.pNext = &features_1_2;
    features_1_2.pNext = &features_1_3;
    vkGetPhysicalDeviceFeatures2(m_parent_physical_device, &features);
    DECLARE_VK_PFN(m_dl, vkCreateDevice);
    DECLARE_VK_PFN(m_dl, vkGetDeviceQueue);
    VK_SUCCEEDS(vkCreateDevice(m_parent_physical_device, &device_info, nullptr, &m_device));
    m_dl.load(m_device);
    vkGetDeviceQueue(m_device, m_graphics_queue_family, 0, &m_graphics_queue);
    vkGetDeviceQueue(m_device, m_transfer_queue_family, 0, &m_transfer_queue);
    vkGetDeviceQueue(m_device, m_present_queue_family, 0, &m_present_queue);
    // Create pipeline cache.
    {
      DECLARE_VK_PFN(m_dl, vkCreatePipelineCache);
      auto cache_info = VkPipelineCacheCreateInfo{ };
      cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
      VK_SUCCEEDS(vkCreatePipelineCache(m_device, &cache_info, nullptr, &m_pipeline_cache));
    }
    // Query presentation modes.
    {
      DECLARE_VK_PFN(m_dl, vkGetPhysicalDeviceSurfacePresentModesKHR);
      auto sz = u32{ };
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfacePresentModesKHR(m_parent_physical_device, m_parent_surface, &sz, nullptr));
      auto present_modes = std::vector<VkPresentModeKHR>(sz);
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfacePresentModesKHR(m_parent_physical_device, m_parent_surface, &sz,
                                                            present_modes.data()));
      for (const auto mode : present_modes)
      {
        m_available_present_modes.insert(mode);
      }
    }
    // This is assuming that There's only one queue family in use.
    // Create command pools.
    {
      auto command_pool_info = VkCommandPoolCreateInfo{ };
      command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
      command_pool_info.queueFamilyIndex = m_graphics_queue_family;
      DECLARE_VK_PFN(m_dl, vkCreateCommandPool);
      for (auto& command_pool : m_command_pools)
      {
        VK_SUCCEEDS(vkCreateCommandPool(m_device, &command_pool_info, nullptr, &command_pool));
      }
    }
    // Create command buffers.
    {
      auto command_buffer_info = VkCommandBufferAllocateInfo{ };
      command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      command_buffer_info.commandBufferCount = 1;
      auto cur = m_command_pools.begin();
      DECLARE_VK_PFN(m_dl, vkAllocateCommandBuffers);
      for (auto& command_buffer : m_command_buffers)
      {
        command_buffer_info.commandPool = *(cur++);
        VK_SUCCEEDS(vkAllocateCommandBuffers(m_device, &command_buffer_info, &command_buffer));
      }
    }
    // Create semaphores.
    {
      auto semaphore_info = VkSemaphoreCreateInfo{ };
      semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      DECLARE_VK_PFN(m_dl, vkCreateSemaphore);
      for (auto& per_frame_semaphores : m_semaphores)
      {
        for (auto& semaphore : per_frame_semaphores)
        {
          VK_SUCCEEDS(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &semaphore));
        }
      }
    }
    // Create fences.
    {
      auto fence_info = VkFenceCreateInfo{ };
      fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      DECLARE_VK_PFN(m_dl, vkCreateFence);
      for (auto& fence : m_fences)
      {
        VK_SUCCEEDS(vkCreateFence(m_device, &fence_info, nullptr, &fence));
      }
    }
    // Create swapchain.
    create_swapchain(desired_image_extent, VK_NULL_HANDLE);
    create_swapchain_image_views();
  }

  vk_device::~vk_device() noexcept {
    DECLARE_VK_PFN(m_dl, vkDeviceWaitIdle);
    vkDeviceWaitIdle(m_device);
    destroy_swapchain_image_views();
    destroy_swapchain(m_swapchain);
    DECLARE_VK_PFN(m_dl, vkDestroyFence);
    for (const auto& fence : m_fences)
    {
      vkDestroyFence(m_device, fence, nullptr);
    }
    DECLARE_VK_PFN(m_dl, vkDestroySemaphore);
    for (const auto& per_frame_semaphores : m_semaphores)
    {
      for (const auto& semaphore : per_frame_semaphores)
      {
        vkDestroySemaphore(m_device, semaphore, nullptr);
      }
    }
    DECLARE_VK_PFN(m_dl, vkFreeCommandBuffers);
    {
      auto cur = m_command_pools.cbegin();
      for (const auto& command_buffer : m_command_buffers)
      {
        vkFreeCommandBuffers(m_device, *(cur++), 1, &command_buffer);
      }
    }
    DECLARE_VK_PFN(m_dl, vkDestroyCommandPool);
    for (const auto& command_pool : m_command_pools)
    {
      vkDestroyCommandPool(m_device, command_pool, nullptr);
    }
    DECLARE_VK_PFN(m_dl, vkDestroyPipeline);
    for (const auto& pipeline : m_interned_graphics_pipelines)
    {
      vkDestroyPipeline(m_device, pipeline, nullptr);
    }
    DECLARE_VK_PFN(m_dl, vkDestroyPipelineLayout);
    for (const auto& layout : m_interned_layouts)
    {
      vkDestroyPipelineLayout(m_device, layout, nullptr);
    }
    DECLARE_VK_PFN(m_dl, vkDestroyPipelineCache);
    vkDestroyPipelineCache(m_device, m_pipeline_cache, nullptr);
    DECLARE_VK_PFN(m_dl, vkDestroyDevice);
    vkDestroyDevice(m_device, nullptr);
  }

  void vk_device::create_swapchain(const VkExtent2D& desired_image_extent, const VkSwapchainKHR old) {
    auto swapchain_info = VkSwapchainCreateInfoKHR{ };
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.clipped = true;
    swapchain_info.surface = m_parent_surface;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    auto capabilities = VkSurfaceCapabilitiesKHR{ };
    DECLARE_VK_PFN(m_dl, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_parent_physical_device, m_parent_surface,
                                                          &capabilities));
    swapchain_info.imageExtent = m_render_area.extent = capabilities.currentExtent;
    // If the current extent is (0xffffffff, 0xffffffff) then the surface extent is controlled by the swapchain
    // extent (rather than the usual case). This is a weird case that I'm pretty sure doesn't happen with X11.
    if (capabilities.currentExtent.width == std::numeric_limits<u32>::max())
    {
      const auto width = std::clamp(desired_image_extent.width, capabilities.maxImageExtent.width,
                                    capabilities.maxImageExtent.width);
      const auto height = std::clamp(desired_image_extent.height, capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);
      swapchain_info.imageExtent.width = m_render_area.extent.width = width;
      swapchain_info.imageExtent.height = m_render_area.extent.height = height;
    }
    select_surface_format(m_dl, m_parent_physical_device, m_parent_surface, m_surface_format);
    swapchain_info.imageFormat = m_surface_format.format;
    swapchain_info.imageColorSpace = m_surface_format.colorSpace;
    // This defaults to VK_PRESENT_MODE_FIFO_KHR.
    // FIFO is the only present mode required to be supported.
    // A different mode can be requested by rebuilding the swapchain later.
    swapchain_info.presentMode = m_present_mode;
    swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    // The maximum image count can be 0 which indicates no maximum. In such a case clamp will fail so we swap out
    // the max with std::numeric_limits<u32>::max() in this case.
    const auto max_image_count = capabilities.maxImageCount +
                                 (-(capabilities.maxImageCount == 0) & std::numeric_limits<u32>::max());
    // This defaults to 3.
    // Triple buffering is usually a safe default.
    // A different buffer count can be requested by rebuilding the swapchain later.
    swapchain_info.minImageCount = std::clamp(m_buffer_count, capabilities.minImageCount, max_image_count);
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // Image array layers are for stereoscopic output.
    // Like a head mounted display.
    swapchain_info.imageArrayLayers = 1;
    // Images are only going to be used on one queue.
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // On the first swapchain creation this is VK_NULL_HANDLE.
    // On recreation this is the previous swapchain ready for tear down.
    swapchain_info.oldSwapchain = old;
    DECLARE_VK_PFN(m_dl, vkCreateSwapchainKHR);
    VK_SUCCEEDS(vkCreateSwapchainKHR(m_device, &swapchain_info, nullptr, &m_swapchain));
    DECLARE_VK_PFN(m_dl, vkGetSwapchainImagesKHR);
    auto sz = u32{ 0 };
    VK_SUCCEEDS(vkGetSwapchainImagesKHR(m_device, m_swapchain, &sz, nullptr));
    m_swapchain_images.resize(sz);
    m_swapchain_image_views.resize(sz);
    m_buffer_count = sz;
    VK_SUCCEEDS(vkGetSwapchainImagesKHR(m_device, m_swapchain, &sz, m_swapchain_images.data()));
  }

  void vk_device::destroy_swapchain(const VkSwapchainKHR swapchain) noexcept {
    DECLARE_VK_PFN(m_dl, vkDestroySwapchainKHR);
    vkDestroySwapchainKHR(m_device, swapchain, nullptr);
    m_swapchain_images.clear();
  }

  void vk_device::create_swapchain_image_views() {
    auto image_view_info = VkImageViewCreateInfo{ };
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.format = m_surface_format.format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.layerCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseMipLevel = 0;
    {
      auto cur = m_swapchain_images.cbegin();
      DECLARE_VK_PFN(m_dl, vkCreateImageView);
      for (auto& image_view : m_swapchain_image_views)
      {
        image_view_info.image = *(cur++);
        VK_SUCCEEDS(vkCreateImageView(m_device, &image_view_info, nullptr, &image_view));
      }
    }
  }

  void vk_device::destroy_swapchain_image_views() noexcept {
    DECLARE_VK_PFN(m_dl, vkDestroyImageView);
    for (const auto& image_view : m_swapchain_image_views)
    {
      vkDestroyImageView(m_device, image_view, nullptr);
    }
    m_swapchain_image_views.clear();
  }

  void vk_device::dirty() {
    m_status |= vk_device_status::dirty_bit;
  }

  void vk_device::recreate_swapchain() {
    DECLARE_VK_PFN(m_dl, vkDeviceWaitIdle);
    VK_SUCCEEDS(vkDeviceWaitIdle(m_device));
    auto old = m_swapchain;
    destroy_swapchain_image_views();
    create_swapchain(m_render_area.extent, old);
    create_swapchain_image_views();
    destroy_swapchain(old);
    m_status &= ~vk_device_status::dirty_bit;
  }

  void vk_device::request_present_mode(const VkPresentModeKHR mode) {
    if (m_available_present_modes.contains(mode))
    {
      m_present_mode = mode;
    }
    dirty();
  }

  VkPresentModeKHR vk_device::current_present_mode() const {
    return m_present_mode;
  }

  void vk_device::request_swapchain_images(const u32 images) {
    m_buffer_count = images;
    dirty();
  }

  u32 vk_device::current_swapchain_size() const {
    return m_buffer_count;
  }

  void vk_device::begin_frame() {
    if (m_status & vk_device_status::dirty_bit)
    {
      recreate_swapchain();
    }
    DECLARE_VK_PFN(m_dl, vkAcquireNextImageKHR);
    {
      auto status = vkAcquireNextImageKHR(m_device, m_swapchain, OBERON_LINUX_VK_FOREVER,
                                          m_semaphores[m_current_frame][SEMAPHORE_IMAGE_ACQUIRED], VK_NULL_HANDLE,
                                          &m_current_image);
      switch (status)
      {
      case VK_SUBOPTIMAL_KHR:
      case VK_ERROR_OUT_OF_DATE_KHR:
        m_status |= vk_device_status::dirty_bit;
      case VK_SUCCESS:
        break;
      default:
        throw vk_error{ "Failed to acquire image for rendering.", status };
      }
    }
    DECLARE_VK_PFN(m_dl, vkWaitForFences);
    DECLARE_VK_PFN(m_dl, vkResetFences);
    VK_SUCCEEDS(vkWaitForFences(m_device, 1, &m_fences[m_current_frame], true, OBERON_LINUX_VK_FOREVER));
    VK_SUCCEEDS(vkResetFences(m_device, 1, &m_fences[m_current_frame]));
    DECLARE_VK_PFN(m_dl, vkResetCommandPool);
    VK_SUCCEEDS(vkResetCommandPool(m_device, m_command_pools[m_current_frame], 0));
    DECLARE_VK_PFN(m_dl, vkBeginCommandBuffer);
    auto begin_info = VkCommandBufferBeginInfo{ };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_SUCCEEDS(vkBeginCommandBuffer(m_command_buffers[m_current_frame], &begin_info));
    {
      DECLARE_VK_PFN(m_dl, vkCmdPipelineBarrier);
      auto image_memory_barrier = VkImageMemoryBarrier{ };
      image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      image_memory_barrier.srcQueueFamilyIndex = -1;
      image_memory_barrier.dstQueueFamilyIndex = -1;
      image_memory_barrier.image = m_swapchain_images[m_current_image];
      image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      image_memory_barrier.subresourceRange.baseMipLevel = 0;
      image_memory_barrier.subresourceRange.levelCount = std::numeric_limits<u32>::max();
      image_memory_barrier.subresourceRange.baseArrayLayer = 0;
      image_memory_barrier.subresourceRange.layerCount = std::numeric_limits<u32>::max();
      // Transition image to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      vkCmdPipelineBarrier(m_command_buffers[m_current_frame], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &image_memory_barrier);
    }
    auto color_attachment_info = VkRenderingAttachmentInfo{ };
    color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // This is a neutral gray color. It's important that the clear color is not black because many render errors will
    // output black pixels.
    // This color is interpretted as being in linear colorspace and not sRGB.
    color_attachment_info.clearValue.color = { { 0.03f, 0.03f, 0.03f, 1.0f } };
    auto rendering_info = VkRenderingInfo{ };
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea = m_render_area;
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment_info;
    color_attachment_info.imageView = m_swapchain_image_views[m_current_image];
    DECLARE_VK_PFN(m_dl, vkCmdBeginRendering);
    vkCmdBeginRendering(m_command_buffers[m_current_frame], &rendering_info);
    DECLARE_VK_PFN(m_dl, vkCmdSetViewport);
    // Set dynamic viewport and scissor.
    auto viewport = VkViewport{ };
    viewport.x = m_render_area.offset.x;
    viewport.y = m_render_area.offset.y;
    viewport.width = m_render_area.extent.width;
    viewport.height = m_render_area.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_command_buffers[m_current_frame], 0, 1, &viewport);
    DECLARE_VK_PFN(m_dl, vkCmdSetScissor);
    vkCmdSetScissor(m_command_buffers[m_current_frame], 0, 1, &m_render_area);
    m_status |= vk_device_status::rendering_bit;
  }

  void vk_device::end_frame() {
    DECLARE_VK_PFN(m_dl, vkCmdEndRendering);
    vkCmdEndRendering(m_command_buffers[m_current_frame]);
    {
      DECLARE_VK_PFN(m_dl, vkCmdPipelineBarrier);
      auto image_memory_barrier = VkImageMemoryBarrier{ };
      image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      image_memory_barrier.srcQueueFamilyIndex = -1;
      image_memory_barrier.dstQueueFamilyIndex = -1;
      image_memory_barrier.image = m_swapchain_images[m_current_image];
      image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      image_memory_barrier.subresourceRange.baseMipLevel = 0;
      image_memory_barrier.subresourceRange.levelCount = std::numeric_limits<u32>::max();
      image_memory_barrier.subresourceRange.baseArrayLayer = 0;
      image_memory_barrier.subresourceRange.layerCount = std::numeric_limits<u32>::max();
      // Transition image to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      vkCmdPipelineBarrier(m_command_buffers[m_current_frame], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &image_memory_barrier);
    }
    DECLARE_VK_PFN(m_dl, vkEndCommandBuffer);
    VK_SUCCEEDS(vkEndCommandBuffer(m_command_buffers[m_current_frame]));
    DECLARE_VK_PFN(m_dl, vkQueueSubmit);
    auto submit_info = VkSubmitInfo{ };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_command_buffers[m_current_frame];
    submit_info.pWaitSemaphores = &m_semaphores[m_current_frame][SEMAPHORE_IMAGE_ACQUIRED];
    submit_info.waitSemaphoreCount = 1;
    auto wait_stage = VkPipelineStageFlags{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.pSignalSemaphores = &m_semaphores[m_current_frame][SEMAPHORE_RENDER_FINISHED];
    submit_info.signalSemaphoreCount = 1;
    // This is slow. Only do it once per queue per frame.
    VK_SUCCEEDS(vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_fences[m_current_frame]));
    auto present_info = VkPresentInfoKHR{ };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pSwapchains = &m_swapchain;
    present_info.swapchainCount = 1;
    present_info.pImageIndices = &m_current_image;
    present_info.pWaitSemaphores = &m_semaphores[m_current_frame][SEMAPHORE_RENDER_FINISHED];
    present_info.waitSemaphoreCount = 1;
    DECLARE_VK_PFN(m_dl, vkQueuePresentKHR);
    {
      auto status = vkQueuePresentKHR(m_present_queue, &present_info);
      switch (status)
      {
      case VK_ERROR_OUT_OF_DATE_KHR:
      case VK_SUBOPTIMAL_KHR:
        m_status |= vk_device_status::dirty_bit;
      case VK_SUCCESS:
        break;
      default:
        throw vk_error{ "Failed to present image.", status };
      }
    }
    // Equivalent to m_frame_index = (m_frame_index + 1) % OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT.
    m_current_frame = (m_current_frame + 1) & (OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT - 1);
    m_status &= ~vk_device_status::rendering_bit;
  }

  VkShaderModule vk_device::create_shader_module(const VkShaderModuleCreateInfo& info) {
    DECLARE_VK_PFN(m_dl, vkCreateShaderModule);
    auto result = VkShaderModule{ };
    VK_SUCCEEDS(vkCreateShaderModule(m_device, &info, nullptr, &result));
    return result;
  }

  void vk_device::destroy_shader_module(const VkShaderModule shader_module) noexcept {
    DECLARE_VK_PFN(m_dl, vkDestroyShaderModule);
    vkDestroyShaderModule(m_device, shader_module, nullptr);
  }

  VkFormat vk_device::current_swapchain_format() const {
    return m_surface_format.format;
  }

  VkPipelineLayout vk_device::intern_pipeline_layout(const VkPipelineLayoutCreateInfo& info) {
    DECLARE_VK_PFN(m_dl, vkCreatePipelineLayout);
    auto layout = VkPipelineLayout{ };
    VK_SUCCEEDS(vkCreatePipelineLayout(m_device, &info, nullptr, &layout));
    m_interned_layouts.emplace_back(layout);
    return layout;
  }

  VkPipeline vk_device::intern_graphics_pipeline(const VkGraphicsPipelineCreateInfo& info) {
    DECLARE_VK_PFN(m_dl, vkCreateGraphicsPipelines);
    auto pipeline = VkPipeline{ };
    VK_SUCCEEDS(vkCreateGraphicsPipelines(m_device, m_pipeline_cache, 1, &info, nullptr, &pipeline));
    m_interned_graphics_pipelines.emplace_back(pipeline);
    return pipeline;
  }

  void vk_device::draw(const VkPipeline pipeline, const u32 vertices) {
    const auto& command_buffer = m_command_buffers[m_current_frame];
    DECLARE_VK_PFN(m_dl, vkCmdBindPipeline);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    DECLARE_VK_PFN(m_dl, vkCmdDraw);
    vkCmdDraw(command_buffer, vertices, 1, 0, 0);
  }

}
