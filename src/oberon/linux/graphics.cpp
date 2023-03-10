#include "oberon/linux/graphics.hpp"

#include <unordered_set>
#include <limits>
#include <algorithm>

#include "oberon/debug.hpp"

#include "oberon/linux/system.hpp"
#include "oberon/linux/window.hpp"
#include "oberon/linux/vk.hpp"

namespace oberon::linux {

  graphics::queue_selection graphics::select_queues_heuristic(const graphics& gfx, const u32,
                                                              const VkPhysicalDevice device) {
    auto& dl = gfx.m_parent->vk_dl();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceQueueFamilyProperties);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceSupportKHR);
    auto sz = u32{ };
    vkGetPhysicalDeviceQueueFamilyProperties(device, &sz, nullptr);
    auto queue_family_properties = std::vector<VkQueueFamilyProperties>(sz);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &sz, queue_family_properties.data());
    auto result = queue_selection{ };
    auto surface_support = VkBool32{ };
    auto gfx_found = false;
    auto present_found = false;
    for (auto i = usize{ 0 }; i < queue_family_properties.size() && (!gfx_found || !present_found); ++i)
    {
      if (!gfx_found && queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        result.graphics_queue = i;
        gfx_found = true;
      }
      OBERON_LINUX_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, gfx.m_target->surface(),
                                                                    &surface_support));
      if (!present_found && surface_support)
      {
        result.presentation_queue = i;
        present_found = true;
      }
    }
    OBERON_POSTCONDITION(gfx_found && present_found);
    return result;
  }

  graphics::queue_selection graphics::select_queues_amd(const graphics&, const u32 vendor, const VkPhysicalDevice) {
    OBERON_PRECONDITION(vendor == OBERON_LINUX_VK_PCI_VENDOR_ID_AMD);
    // AMD presents a Graphics/Transfer/Compute/Present queue family as family 0.
    return { 0, 0 };
  }

  graphics::queue_selection graphics::select_queues_nvidia(const graphics&, const u32 vendor, const VkPhysicalDevice) {
    OBERON_PRECONDITION(vendor == OBERON_LINUX_VK_PCI_VENDOR_ID_NVIDIA);
    // Nvidia is a little different from other vendors in that they provide 16 queues in family 0.
    // Otherwise though, it's similar to AMD where there is a Graphics/Transfer/Compute/Present queue family as
    // family 0
    return { 0, 0 };
  }

  graphics::queue_selection graphics::select_queues_intel(const graphics&, const u32 vendor, const VkPhysicalDevice) {
    OBERON_PRECONDITION(vendor == OBERON_LINUX_VK_PCI_VENDOR_ID_INTEL);
    // Intel only provides one queue family and it's a Graphics/Transfer/Compute/Present queue family.
    return { 0, 0 };
  }



  graphics::graphics(system& sys, window& win) : m_parent{ &sys }, m_target{ &win } {
    auto& dl = m_parent->vk_dl();
    auto instance = m_parent->instance();
    // Load physical devices.
    auto all_physical_devices = std::vector<VkPhysicalDevice>{ };
    {
      auto sz = u32{ };
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkEnumeratePhysicalDevices);
      OBERON_LINUX_VK_SUCCEEDS(vkEnumeratePhysicalDevices(instance, &sz, nullptr));
      all_physical_devices.resize(sz);
      OBERON_LINUX_VK_SUCCEEDS(vkEnumeratePhysicalDevices(instance, &sz, all_physical_devices.data()));
    }
    // Filter out physical devices lacking basic capabilities.
    {
      auto sz = u32{ };
      auto physical_device_properties2 = VkPhysicalDeviceProperties2{ };
      physical_device_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
      auto physical_device_properties12 = VkPhysicalDeviceVulkan12Properties{ };
      physical_device_properties12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
      physical_device_properties2.pNext = &physical_device_properties12;
      auto queue_family_properties2 = std::vector<VkQueueFamilyProperties2>{ };
      auto physical_device_features2 = VkPhysicalDeviceFeatures2{ };
      physical_device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
      auto physical_device_features13 = VkPhysicalDeviceVulkan13Features{ };
      physical_device_features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
      physical_device_features2.pNext = &physical_device_features13;
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceProperties2);
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceQueueFamilyProperties2);
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceSupportKHR);
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceFeatures2);
      // In general, device selection is going to involve a lot of nested loops like this.
      // It's important to keep in mind that the length of many of these arrays will be 1, 2, or 3.
      // It is uncommon, for example, for consumer systems to have more than one actual discrete GPU. A system
      // running Mesa with integrated and discrete GPUs may report 3 (discrete GPU, integrated GPU, llvmpipe)
      // It's also uncommon for discrete GPUs to present dozens of acceptable queue families.
      // For example, my (AMD) GPU presents only one family capable of graphics operations and only one queue in that
      // family.
      // tl;dr Most of these loops will run 3 times max.
      for (const auto& physical_device : all_physical_devices)
      {
        vkGetPhysicalDeviceProperties2(physical_device, &physical_device_properties2);
        vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &sz, nullptr);
        queue_family_properties2.resize(sz);
        for (auto& queue_family_property : queue_family_properties2)
        {
          queue_family_property.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }
        vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &sz, queue_family_properties2.data());
        // Check for Vulkan 1.3 support.
        auto major = VK_API_VERSION_MAJOR(physical_device_properties2.properties.apiVersion);
        auto minor = VK_API_VERSION_MINOR(physical_device_properties2.properties.apiVersion);
        // Check support for graphics and presentation on window surface.
        auto has_gfx_queue_family = false;
        auto has_present_queue_family = false;
        sz = 0;
        for (const auto& queue_family_property : queue_family_properties2)
        {
          const auto& properties = queue_family_property.queueFamilyProperties;
          has_gfx_queue_family = has_gfx_queue_family || properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
          auto surface_support = VkBool32{ };
          OBERON_LINUX_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, sz, m_target->surface(),
                                                                        &surface_support));
          has_present_queue_family = has_present_queue_family || surface_support;
          ++sz;
        }
        // Check support for dynamic rendering.
        vkGetPhysicalDeviceFeatures2(physical_device, &physical_device_features2);
        if ((major == 1 && minor < 3) || !physical_device_features13.dynamicRendering || !has_gfx_queue_family ||
            !has_present_queue_family)
        {
          continue;
        }
        m_physical_devices.emplace_back(physical_device);
        {
          auto type = static_cast<graphics_device_type>(physical_device_properties2.properties.deviceType);
          auto handle = reinterpret_cast<uptr>(&m_physical_devices.back());
          auto name = std::string{ physical_device_properties2.properties.deviceName };
          auto driver_name = std::string{ physical_device_properties12.driverName };
          auto driver_info = std::string{ physical_device_properties12.driverInfo };
          m_graphics_devices.emplace_back(graphics_device{ type, handle, name, driver_name, driver_info });
        }
      }
    }
    initialize_device(m_physical_devices.front());
    initialize_renderer(VK_NULL_HANDLE);
  }

  void graphics::initialize_device(const VkPhysicalDevice device) {
    auto device_info = VkDeviceCreateInfo{ };
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    auto& dl = m_parent->vk_dl();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceFeatures2);
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
    vkGetPhysicalDeviceFeatures2(device, &physical_device_features);
    device_info.pNext = &physical_device_features;
    device_info.pEnabledFeatures = nullptr;
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceProperties);
    auto queue_selectors = std::array<queue_selection(*)(const graphics&, const u32, const VkPhysicalDevice), 4>{
      select_queues_heuristic, select_queues_amd, select_queues_nvidia, select_queues_intel
    };
    auto properties = VkPhysicalDeviceProperties{ };
    vkGetPhysicalDeviceProperties(device, &properties);
    // Branchless selection of correct vendor:
    // 0x1002 = AMD
    // 0x10de = Nvidia
    // 0x8086 = Intel
    // Anything else defaults to the heuristic approach.
    const auto index = (1 & -(properties.vendorID == OBERON_LINUX_VK_PCI_VENDOR_ID_AMD)) +
                       (2 & -(properties.vendorID == OBERON_LINUX_VK_PCI_VENDOR_ID_NVIDIA)) +
                       (3 & -(properties.vendorID == OBERON_LINUX_VK_PCI_VENDOR_ID_INTEL));
    m_selected_queue_families = queue_selectors[index](*this, properties.vendorID, device);
    auto multi_queue = m_selected_queue_families.graphics_queue != m_selected_queue_families.presentation_queue;
    // 1 + static_cast<usize>(multi_queue) is either 1 or 2.
    auto queue_infos = std::vector<VkDeviceQueueCreateInfo>(1 + multi_queue);
    auto priority = 1.0f;
    queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_infos[0].queueFamilyIndex = m_selected_queue_families.graphics_queue;
    queue_infos[0].queueCount = 1;
    queue_infos[0].pQueuePriorities = &priority;
    if (multi_queue)
    {
      queue_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_infos[1].queueFamilyIndex = m_selected_queue_families.presentation_queue;
      queue_infos[1].queueCount = 1;
      queue_infos[1].pQueuePriorities = &priority;
    }
    device_info.pQueueCreateInfos = queue_infos.data();
    device_info.queueCreateInfoCount = queue_infos.size();
    auto available_extensions = std::unordered_set<std::string>{ };
    {
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkEnumerateDeviceExtensionProperties);
      auto sz = u32{ };
      OBERON_LINUX_VK_SUCCEEDS(vkEnumerateDeviceExtensionProperties(device, nullptr, &sz, nullptr));
      auto extension_properties = std::vector<VkExtensionProperties>(sz);
      OBERON_LINUX_VK_SUCCEEDS(vkEnumerateDeviceExtensionProperties(device, nullptr, &sz,
                                                                    extension_properties.data()));
      for (const auto& extension_property : extension_properties)
      {
        available_extensions.insert(extension_property.extensionName);
      }
    }
    // Select extensions and check that required extensions are available.
    auto selected_extensions = std::vector<cstring>{ };
    {
#define OBERON_LINUX_VK_REQUIRE_EXTENSION(ext) \
  do \
  { \
    if (!available_extensions.contains((ext))) \
    { \
      throw vk_error { "The selected Vulkan device does not support \"" ext "\"", 1 }; \
    } \
    selected_extensions.emplace_back((ext)); \
  } \
  while (0)
      OBERON_LINUX_VK_REQUIRE_EXTENSION(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
      OBERON_LINUX_VK_REQUIRE_EXTENSION(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
      // This has to be true according to the specification
      // see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceCreateInfo.html
      available_extensions.erase(VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME);
      if (available_extensions.contains(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
      {
        available_extensions.erase(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
      }
      // The specification lists a whole bunch of checks for when certain extensions are selected but the corresponding
      // feature is not enabled. I'm going to just assume (since I enable everything that is available) that
      // everything is fine.
#undef OBERON_LINUX_VK_REQUIRE_EXTENSION
    }
    device_info.ppEnabledExtensionNames = selected_extensions.data();
    device_info.enabledExtensionCount = selected_extensions.size();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCreateDevice);
    OBERON_LINUX_VK_SUCCEEDS(vkCreateDevice(device, &device_info, nullptr, &m_vk_device));
    dl.load(m_vk_device);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetDeviceQueue);
    vkGetDeviceQueue(m_vk_device, m_selected_queue_families.graphics_queue, 0, &m_vk_graphics_queue);
    vkGetDeviceQueue(m_vk_device, m_selected_queue_families.presentation_queue, 0, &m_vk_present_queue);
    m_vk_selected_physical_device = device;
    {
      auto command_pool_info = VkCommandPoolCreateInfo{ };
      command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      command_pool_info.queueFamilyIndex = m_selected_queue_families.graphics_queue;
      command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkCreateCommandPool);
      OBERON_LINUX_VK_SUCCEEDS(vkCreateCommandPool(m_vk_device, &command_pool_info, nullptr, &m_vk_command_pool));
      auto command_buffer_info = VkCommandBufferAllocateInfo{ };
      command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      command_buffer_info.commandPool = m_vk_command_pool;
      command_buffer_info.commandBufferCount = m_vk_command_buffers.size();
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkAllocateCommandBuffers);
      OBERON_LINUX_VK_SUCCEEDS(vkAllocateCommandBuffers(m_vk_device, &command_buffer_info,
                                                        m_vk_command_buffers.data()));
    }
    {
      auto semaphore_info = VkSemaphoreCreateInfo{ };
      semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      auto fence_info = VkFenceCreateInfo{ };
      fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkCreateSemaphore);
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkCreateFence);
      for (auto i = usize{ 0 }; i < OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT; ++i)
      {
        OBERON_LINUX_VK_SUCCEEDS(vkCreateSemaphore(m_vk_device, &semaphore_info, nullptr,
                                                   &m_vk_image_available_sems[i]));
        OBERON_LINUX_VK_SUCCEEDS(vkCreateSemaphore(m_vk_device, &semaphore_info, nullptr,
                                                   &m_vk_render_finished_sems[i]));
        OBERON_LINUX_VK_SUCCEEDS(vkCreateFence(m_vk_device, &fence_info, nullptr,
                                              &m_vk_in_flight_frame_fences[i]));
      }
    }
  }

  void graphics::deinitialize_device() {
    const auto& dl = m_parent->vk_dl();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkDestroyPipeline);
    for (const auto& pipeline : m_vk_graphics_pipelines)
    {
      vkDestroyPipeline(m_vk_device, pipeline, nullptr);
    }
    m_vk_graphics_pipelines.clear();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkDestroyPipelineLayout);
    for (const auto& layout : m_vk_graphics_pipeline_layouts)
    {
      vkDestroyPipelineLayout(m_vk_device, layout, nullptr);
    }
    m_vk_graphics_pipeline_layouts.clear();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkDestroyShaderModule);
    for (const auto& shader_stage : m_vk_shader_stages)
    {
      vkDestroyShaderModule(m_vk_device, shader_stage.module, nullptr);
    }
    m_vk_shader_stages.clear();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkDestroySemaphore);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkDestroyFence);
    for (auto i = usize{ 0 }; i < OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT; ++i)
    {
      vkDestroySemaphore(m_vk_device, m_vk_image_available_sems[i], nullptr);
      vkDestroySemaphore(m_vk_device, m_vk_render_finished_sems[i], nullptr);
      vkDestroyFence(m_vk_device, m_vk_in_flight_frame_fences[i], nullptr);
    }
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkFreeCommandBuffers);
    vkFreeCommandBuffers(m_vk_device, m_vk_command_pool, m_vk_command_buffers.size(), m_vk_command_buffers.data());
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkDestroyCommandPool);
    vkDestroyCommandPool(m_vk_device, m_vk_command_pool, nullptr);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkDestroyDevice);
    vkDestroyDevice(m_vk_device, nullptr);
  }

  void graphics::initialize_renderer(const VkSwapchainKHR old) {
    auto& dl = m_parent->vk_dl();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    auto capabilities = VkSurfaceCapabilitiesKHR{ };
    OBERON_LINUX_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vk_selected_physical_device,
                                                                       m_target->surface(), &capabilities));
    auto swapchain_info = VkSwapchainCreateInfoKHR{ };
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = m_target->surface();
    // TODO: allow users to select between double and triple buffering.
    // maxImageCount can be 0 to indicate no limits
    auto sz = capabilities.maxImageCount;
    swapchain_info.minImageCount = std::clamp(OBERON_LINUX_VK_TRIPLE_BUFFER_IMAGE_COUNT, capabilities.minImageCount,
                                              (std::numeric_limits<u32>::max() & -!sz) + sz);
    // Find image format
    {
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceFormatsKHR);
      OBERON_LINUX_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(m_vk_selected_physical_device,
                                                                    swapchain_info.surface, &sz, nullptr));
      auto formats = std::vector<VkSurfaceFormatKHR>(sz);
      OBERON_LINUX_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(m_vk_selected_physical_device,
                                                                    swapchain_info.surface, &sz, formats.data()));
      auto found = false;
      // On my system there are exactly 2 available surface formats (i.e., image formats and color spaces that can
      // be displayed on the screen). These are B8G8R8A8_SRGB with COLOR_SPACE_SRGB_NONLINEAR_KHR and B8G8R8A8_UNORM
      // with COLOR_SPACE_SRGB_NONLINEAR_KHR. I don't know if other devices and window systems support vastly more
      // surface formats but it seems doubtful. In any case, this searches for a common surface format (gpuinfo.org
      // reports that B8G8R8A8_SRGB is available on 62% of devices). If that surface format isn't found it defaults
      // to the first reported format.
      for (const auto& format : formats)
      {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
          swapchain_info.imageFormat = format.format;
          swapchain_info.imageColorSpace = format.colorSpace;
          found = true;
          break;
        }
      }
      if (!found)
      {
        swapchain_info.imageFormat = formats.front().format;
        swapchain_info.imageColorSpace = formats.front().colorSpace;
      }
    }
    auto drawable_area = m_target->current_drawable_rect();
    // Unreasonable hypothetical scenario described in the Vulkan specification where the surface size is determined
    // by the swapchain and not by the window system. That is to say, the case where the window resizes to fit
    // the swapchain.
    if (capabilities.currentExtent.width == std::numeric_limits<u32>::max() &&
        capabilities.currentExtent.height == std::numeric_limits<u32>::max())
    {
      swapchain_info.imageExtent.width = std::clamp(static_cast<u32>(drawable_area.extent.width),
                                                    capabilities.minImageExtent.width,
                                                    capabilities.maxImageExtent.width);
      swapchain_info.imageExtent.height = std::clamp(static_cast<u32>(drawable_area.extent.height),
                                                     capabilities.minImageExtent.height,
                                                     capabilities.maxImageExtent.height);
    }
    // More realistic scenario where the swapchain size is the surface size.
    else
    {
      swapchain_info.imageExtent.width = capabilities.currentExtent.width;
      swapchain_info.imageExtent.height = capabilities.currentExtent.height;
    }
    m_vk_render_area.extent = swapchain_info.imageExtent;

    // Array layers are for head mounted displays and other stereoscopic environments.
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // Branchless selection of mode
    // If both queues are the same then mask = 0 otherwise it equals -1 (i.e., 0xff'ff'ff'ff)
    // VK_SHARING_MODE_EXCLUSIVE = 0
    // VK_SHARING_MODE_CONCURRENT = 1
    const auto mask = -(m_selected_queue_families.graphics_queue != m_selected_queue_families.presentation_queue);
    swapchain_info.imageSharingMode = static_cast<VkSharingMode>(VK_SHARING_MODE_CONCURRENT & mask);
    // If concurrent sharing
    if (mask)
    {
      swapchain_info.queueFamilyIndexCount = 2;
      auto queue_indices = std::vector<u32>(swapchain_info.queueFamilyIndexCount);
      queue_indices[0] = m_selected_queue_families.graphics_queue;
      queue_indices[1] = m_selected_queue_families.presentation_queue;
      swapchain_info.pQueueFamilyIndices = queue_indices.data();
    }
    swapchain_info.preTransform = capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // Only FIFO is guaranteed to be available.
    // TODO: allow the present mode to be selected.
    swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_info.clipped = true;
    swapchain_info.oldSwapchain = old;
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCreateSwapchainKHR);
    OBERON_LINUX_VK_SUCCEEDS(vkCreateSwapchainKHR(m_vk_device, &swapchain_info, nullptr, &m_vk_swapchain));
    m_vk_swapchain_image_format = swapchain_info.imageFormat;
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetSwapchainImagesKHR);
    OBERON_LINUX_VK_SUCCEEDS(vkGetSwapchainImagesKHR(m_vk_device, m_vk_swapchain, &sz, nullptr));
    m_vk_swapchain_images.resize(sz);
    m_vk_swapchain_image_views.resize(sz);
    OBERON_LINUX_VK_SUCCEEDS(vkGetSwapchainImagesKHR(m_vk_device, m_vk_swapchain, &sz, m_vk_swapchain_images.data()));
    auto image_view_info = VkImageViewCreateInfo{ };
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.format = swapchain_info.imageFormat;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCreateImageView);
    {
      auto cur = m_vk_swapchain_image_views.begin();
      for (const auto& image : m_vk_swapchain_images)
      {
        image_view_info.image = image;
        OBERON_LINUX_VK_SUCCEEDS(vkCreateImageView(m_vk_device, &image_view_info, nullptr, &(*cur)));
        ++cur;
      }
    }
  }

  // This is probably bad. It's easy to miss this.
  // TODO: refactor this
  void graphics::deinitialize_image_views() {
    const auto& dl = m_parent->vk_dl();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkDestroyImageView);
    for (auto& image_view : m_vk_swapchain_image_views)
    {
      vkDestroyImageView(m_vk_device, image_view, nullptr);
    }
  }

  void graphics::deinitialize_renderer(const VkSwapchainKHR old) {
    const auto& dl = m_parent->vk_dl();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkDestroySwapchainKHR);
    vkDestroySwapchainKHR(m_vk_device, old, nullptr);
  }

  void graphics::deinitialize_renderer() {
    deinitialize_renderer(m_vk_swapchain);
  }

  graphics::~graphics() noexcept {
    wait_for_device_to_idle();
    deinitialize_image_views();
    deinitialize_renderer();
    deinitialize_device();
  }

  const std::vector<graphics_device>& graphics::available_devices() const {
    return m_graphics_devices;
  }

  void graphics::select_device(const graphics_device& device) {
    wait_for_device_to_idle();
    deinitialize_image_views();
    deinitialize_renderer();
    deinitialize_device();
    initialize_device(*reinterpret_cast<ptr<VkPhysicalDevice>>(device.handle));
    initialize_renderer(VK_NULL_HANDLE);
  }

  void graphics::wait_for_device_to_idle() {
    OBERON_LINUX_VK_DECLARE_PFN(m_parent->vk_dl(), vkDeviceWaitIdle);
    vkDeviceWaitIdle(m_vk_device);
  }

  void graphics::reinitialize_renderer() {
    wait_for_device_to_idle();
    auto old = m_vk_swapchain;
    deinitialize_image_views();
    initialize_renderer(old);
    deinitialize_renderer(old);
    m_vk_last_frame = VK_SUCCESS;
  }

  void graphics::begin_frame() {
    const auto& dl = m_parent->vk_dl();
    if (m_vk_last_frame == VK_ERROR_OUT_OF_DATE_KHR || m_vk_last_frame == VK_SUBOPTIMAL_KHR)
    {
      reinitialize_renderer();
    }
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkWaitForFences);
    constexpr const auto FOREVER = std::numeric_limits<u64>::max();
    OBERON_LINUX_VK_SUCCEEDS(vkWaitForFences(m_vk_device, 1, &m_vk_in_flight_frame_fences[m_frame_index], true,
                                             FOREVER));
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkResetFences);
    OBERON_LINUX_VK_SUCCEEDS(vkResetFences(m_vk_device, 1, &m_vk_in_flight_frame_fences[m_frame_index]));
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkAcquireNextImageKHR);
    m_vk_last_frame = vkAcquireNextImageKHR(m_vk_device, m_vk_swapchain, FOREVER,
                                            m_vk_image_available_sems[m_frame_index], VK_NULL_HANDLE, &m_image_index);
    switch (m_vk_last_frame)
    {
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
    case VK_SUCCESS:
      break;
    default:
      throw vk_error{ "Failed to acquire image for rendering.", m_vk_last_frame };
    }
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkBeginCommandBuffer);
    auto begin_info = VkCommandBufferBeginInfo{ };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    OBERON_LINUX_VK_SUCCEEDS(vkBeginCommandBuffer(m_vk_command_buffers[m_frame_index], &begin_info));
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCmdPipelineBarrier);
    auto image_memory_barrier = VkImageMemoryBarrier{ };
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_memory_barrier.srcQueueFamilyIndex = -1;
    image_memory_barrier.dstQueueFamilyIndex = -1;
    image_memory_barrier.image = m_vk_swapchain_images[m_image_index];
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = UINT32_MAX;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = UINT32_MAX;;
    vkCmdPipelineBarrier(m_vk_command_buffers[m_frame_index], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &image_memory_barrier);
    auto color_attachment_info = VkRenderingAttachmentInfo{ };
    color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // Neutral gray
    color_attachment_info.clearValue.color = { { 0.2f, 0.2f, 0.2f, 1.0f } };
    auto rendering_info = VkRenderingInfo{ };
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea = m_vk_render_area;
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment_info;
    color_attachment_info.imageView = m_vk_swapchain_image_views[m_image_index];
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCmdBeginRendering);
    vkCmdBeginRendering(m_vk_command_buffers[m_frame_index], &rendering_info);
  }

  void graphics::end_frame() {
    const auto& dl = m_parent->vk_dl();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCmdEndRendering);
    vkCmdEndRendering(m_vk_command_buffers[m_frame_index]);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCmdPipelineBarrier);
    auto image_memory_barrier = VkImageMemoryBarrier{ };
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    image_memory_barrier.srcQueueFamilyIndex = -1;
    image_memory_barrier.dstQueueFamilyIndex = -1;
    image_memory_barrier.image = m_vk_swapchain_images[m_image_index];
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = UINT32_MAX;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = UINT32_MAX;
    vkCmdPipelineBarrier(m_vk_command_buffers[m_frame_index], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkEndCommandBuffer);
    OBERON_LINUX_VK_SUCCEEDS(vkEndCommandBuffer(m_vk_command_buffers[m_frame_index]));
    auto submit_info = VkSubmitInfo{ };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_vk_command_buffers[m_frame_index];
    submit_info.pWaitSemaphores = &m_vk_image_available_sems[m_frame_index];
    submit_info.waitSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &m_vk_render_finished_sems[m_frame_index];
    submit_info.signalSemaphoreCount = 1;
    auto wait_dst_stage = VkPipelineStageFlags{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = &wait_dst_stage;
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkQueueSubmit);
    OBERON_LINUX_VK_SUCCEEDS(vkQueueSubmit(m_vk_graphics_queue, 1, &submit_info,
                                           m_vk_in_flight_frame_fences[m_frame_index]));
    auto present_info = VkPresentInfoKHR{ };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pSwapchains = &m_vk_swapchain;
    present_info.swapchainCount = 1;
    present_info.pImageIndices = &m_image_index;
    present_info.pWaitSemaphores = &m_vk_render_finished_sems[m_frame_index];
    present_info.waitSemaphoreCount = 1;
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkQueuePresentKHR);
    m_vk_last_frame = vkQueuePresentKHR(m_vk_present_queue, &present_info);
    switch (m_vk_last_frame)
    {
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
    case VK_SUCCESS:
      break;
    default:
      throw vk_error{ "Failed to present image.", m_vk_last_frame };
    }
    m_frame_index = (m_frame_index + 1) & (OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT - 1);
  }

  pipeline_stage_binary graphics::intern_pipeline_stage_binary(const pipeline_stage stage,
                                                               const readonly_ptr<char> bin, const usize sz) {
    // 4 divides sz
    OBERON_PRECONDITION(!(sz & 3));
    OBERON_PRECONDITION(stage != pipeline_stage::none);
    const auto& dl = m_parent->vk_dl();
    auto shader_info = VkShaderModuleCreateInfo{ };
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.pCode = reinterpret_cast<readonly_ptr<u32>>(bin);
    shader_info.codeSize = sz;
    auto pipeline_info = VkPipelineShaderStageCreateInfo{ };
    pipeline_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage = static_cast<VkShaderStageFlagBits>(stage);
    pipeline_info.pName = "main";
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCreateShaderModule);
    OBERON_LINUX_VK_SUCCEEDS(vkCreateShaderModule(m_vk_device, &shader_info, nullptr, &pipeline_info.module));
    m_vk_shader_stages.push_back(pipeline_info);
    return { stage, m_vk_shader_stages.size() - 1 };
  }

  usize graphics::intern_render_program(const render_program& program) {
    const auto& dl = m_parent->vk_dl();
    auto graphics_pipeline_info = VkGraphicsPipelineCreateInfo{ };
    graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    auto pipeline_rendering_info = VkPipelineRenderingCreateInfo{ };
    pipeline_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &m_vk_swapchain_image_format;
    graphics_pipeline_info.pNext = &pipeline_rendering_info;
    auto selected_stages = std::vector<VkPipelineShaderStageCreateInfo>{ };
    {
      const auto stages_as_array = reinterpret_cast<readonly_ptr<usize>>(&program);
      constexpr const auto SUPPORTED_STAGE_COUNT = sizeof(render_program) / sizeof(usize);
      for (auto i = usize{ 0 }; i < SUPPORTED_STAGE_COUNT; ++i)
      {
        OBERON_CHECK(stages_as_array[i] < m_vk_shader_stages.size());
        selected_stages.push_back(m_vk_shader_stages[stages_as_array[i]]);
      }
    }
    graphics_pipeline_info.stageCount = selected_stages.size();
    graphics_pipeline_info.pStages = selected_stages.data();
    auto vertex_input = VkPipelineVertexInputStateCreateInfo{ };
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // TODO: useful vertex inputs
    graphics_pipeline_info.pVertexInputState = &vertex_input;
    auto input_assembly = VkPipelineInputAssemblyStateCreateInfo{ };
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = false;
    graphics_pipeline_info.pInputAssemblyState = &input_assembly;
    // TODO: tesselation
    auto viewport = VkPipelineViewportStateCreateInfo{ };
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;
    graphics_pipeline_info.pViewportState = &viewport;
    auto rasterization = VkPipelineRasterizationStateCreateInfo{ };
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.lineWidth = 1.0f;
    rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    graphics_pipeline_info.pRasterizationState = &rasterization;
    auto multisample = VkPipelineMultisampleStateCreateInfo{ };
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample.minSampleShading = 1.0f;
    graphics_pipeline_info.pMultisampleState = &multisample;
    // TODO: depth_stencil
    auto color_blend = VkPipelineColorBlendStateCreateInfo{ };
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.attachmentCount = 1;
    auto color_blend_attachment = VkPipelineColorBlendAttachmentState{ };
    color_blend_attachment.blendEnable = true;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend.pAttachments = &color_blend_attachment;
    graphics_pipeline_info.pColorBlendState = &color_blend;
    auto dynamic_state = VkPipelineDynamicStateCreateInfo{ };
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    auto dynamic_states = std::array<VkDynamicState, 2>{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    dynamic_state.dynamicStateCount = dynamic_states.size();
    dynamic_state.pDynamicStates = dynamic_states.data();
    graphics_pipeline_info.pDynamicState = &dynamic_state;
    // TODO: obviously some uniforms would be useful.
    auto layout_info = VkPipelineLayoutCreateInfo{ };
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCreatePipelineLayout);
    OBERON_LINUX_VK_SUCCEEDS(vkCreatePipelineLayout(m_vk_device, &layout_info, nullptr,
                                                    &graphics_pipeline_info.layout));
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCreateGraphicsPipelines);
    auto pipeline = VkPipeline{ };
    OBERON_LINUX_VK_SUCCEEDS(vkCreateGraphicsPipelines(m_vk_device, VK_NULL_HANDLE, 1, &graphics_pipeline_info,
                                                       nullptr, &pipeline));
    m_vk_graphics_pipelines.push_back(pipeline);
    m_vk_graphics_pipeline_layouts.push_back(graphics_pipeline_info.layout);
    return m_vk_graphics_pipelines.size() - 1;
  }

  void graphics::draw(const usize vertices, const usize program) {
    const auto& dl = m_parent->vk_dl();
    auto& command_buffer = m_vk_command_buffers[m_frame_index];
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCmdSetViewport);
    auto viewport = VkViewport{ };
    viewport.x = m_vk_render_area.offset.x;
    viewport.y = m_vk_render_area.offset.y;
    viewport.width = m_vk_render_area.extent.width;
    viewport.height = m_vk_render_area.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCmdSetScissor);
    vkCmdSetScissor(command_buffer, 0, 1, &m_vk_render_area);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCmdBindPipeline);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vk_graphics_pipelines[program]);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCmdDraw);
    vkCmdDraw(command_buffer, vertices, 1, 0, 0);
  }

}
