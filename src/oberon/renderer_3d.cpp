#include "oberon/detail/renderer_3d_impl.hpp"

#include <cstring>

#include <limits>

#include "oberon/errors.hpp"
#include "oberon/debug.hpp"

#include "oberon/detail/context_impl.hpp"
#include "oberon/detail/window_impl.hpp"

namespace oberon {
namespace detail {

  iresult retrieve_vulkan_surface_info(
    const context_impl& ctx,
    const window_impl& win,
    renderer_3d_impl& rnd
  ) noexcept {
    OBERON_PRECONDITION(ctx.physical_device);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetPhysicalDeviceSurfaceFormatsKHR);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetPhysicalDeviceSurfacePresentModesKHR);
    OBERON_PRECONDITION(win.surface);

    auto result =
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physical_device, win.surface, &rnd.surface_capabilities);
    OBERON_ASSERT(result == VK_SUCCESS);
    auto sz = u32{ 0 };
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physical_device, win.surface, &sz, nullptr);
    OBERON_ASSERT(result == VK_SUCCESS);
    rnd.surface_formats.resize(sz);
    result =
      vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physical_device, win.surface, &sz, std::data(rnd.surface_formats));
    OBERON_ASSERT(result == VK_SUCCESS);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physical_device, win.surface, &sz, nullptr);
    OBERON_ASSERT(result == VK_SUCCESS);
    rnd.presentation_modes.resize(sz);
    auto data = std::data(rnd.presentation_modes);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physical_device, win.surface, &sz, data);
    OBERON_ASSERT(result == VK_SUCCESS);
    return 0;
 }

namespace {

  VkSurfaceFormatKHR select_surface_format(const std::vector<VkSurfaceFormatKHR>& surface_formats) {
    auto criteria = [](const VkSurfaceFormatKHR& surface_format) {
      if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
          surface_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
      {
        return true;
      }
      return false;
    };
    auto pos = std::find_if(std::begin(surface_formats), std::end(surface_formats), criteria);
    if (pos == std::end(surface_formats))
    {
      return surface_formats.front();
    }
    return *pos;
  }

}
  iresult create_vulkan_swapchain(const context_impl& ctx, const window_impl& win, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.physical_device);
    OBERON_PRECONDITION(ctx.device);
    OBERON_PRECONDITION(win.surface);
    OBERON_PRECONDITION(std::size(rnd.presentation_modes) > 0);
    OBERON_PRECONDITION(std::size(rnd.surface_formats) > 0);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetPhysicalDeviceSurfaceSupportKHR);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateSwapchainKHR);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetSwapchainImagesKHR);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateImageView);

    // Recheck surface support. This is dumb but required by Vulkan.
    {
      auto support = VkBool32{ };
      auto presentation_queue_family = ctx.presentation_queue_family;
      auto result =
        vkGetPhysicalDeviceSurfaceSupportKHR(ctx.physical_device, presentation_queue_family, win.surface, &support);
      OBERON_ASSERT(result == VK_SUCCESS);
      if (!support)
      {
        return -1;
      }
    }
    auto swapchain_info = VkSwapchainCreateInfoKHR{ };
    OBERON_INIT_VK_STRUCT(swapchain_info, SWAPCHAIN_CREATE_INFO_KHR);
    swapchain_info.surface = win.surface;
    swapchain_info.minImageCount = rnd.surface_capabilities.minImageCount + 1;
    if (rnd.surface_capabilities.maxImageCount && swapchain_info.minImageCount > rnd.surface_capabilities.maxImageCount)
    {
      swapchain_info.minImageCount = rnd.surface_capabilities.maxImageCount;
    }
    // FIFO mode support is required by standard.
    // This should be selected by user input instead of the library.
    // Personally I think offer these as FIFO, FIFO Relaxed, Immediate, and Mailbox are better than
    // Offering them as Vsync, Double/triple buffering, etc.
    swapchain_info.presentMode = rnd.current_presentation_mode;
    // This will probably be finicky.
    if (rnd.current_surface_format.format == VK_FORMAT_UNDEFINED)
    {
      rnd.current_surface_format = select_surface_format(rnd.surface_formats);
    }
    swapchain_info.imageFormat = rnd.current_surface_format.format;
    swapchain_info.imageColorSpace = rnd.current_surface_format.colorSpace;
    if (rnd.surface_capabilities.currentExtent.width != std::numeric_limits<u32>::max())
    {
      rnd.current_swapchain_extent = rnd.surface_capabilities.currentExtent;
    }
    else
    {
      auto& capabilities = rnd.surface_capabilities;
      auto actual_extent = VkExtent2D{
        static_cast<u32>(win.bounds.size.width), static_cast<u32>(win.bounds.size.height)
      };
      rnd.current_swapchain_extent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      rnd.current_swapchain_extent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    swapchain_info.imageExtent = rnd.current_swapchain_extent;
    // More is for head mounted displays?
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (ctx.graphics_transfer_queue_family == ctx.presentation_queue_family)
    {
      swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
      auto queue_family_indices =
        std::array<u32, 2>{ ctx.graphics_transfer_queue_family, ctx.presentation_queue_family };
      swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      swapchain_info.pQueueFamilyIndices = std::data(queue_family_indices);
      swapchain_info.queueFamilyIndexCount = std::size(queue_family_indices);
    }
    swapchain_info.preTransform = rnd.surface_capabilities.currentTransform;
    // Should this ever be any other value?
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.clipped = true;
    if (auto result = vkCreateSwapchainKHR(ctx.device, &swapchain_info, nullptr, &rnd.swapchain);
        result != VK_SUCCESS)
    {
      return result;
    }
    {
      auto sz = u32{ 0 };
      auto result = vkGetSwapchainImagesKHR(ctx.device, rnd.swapchain, &sz, nullptr);
      OBERON_ASSERT(result == VK_SUCCESS);
      rnd.swapchain_images.resize(sz);
      result = vkGetSwapchainImagesKHR(ctx.device, rnd.swapchain, &sz, std::data(rnd.swapchain_images));
      OBERON_ASSERT(result == VK_SUCCESS);
    }
    {
      rnd.swapchain_image_views.resize(std::size(rnd.swapchain_images));
      auto image_view_info = VkImageViewCreateInfo{ };
      OBERON_INIT_VK_STRUCT(image_view_info, IMAGE_VIEW_CREATE_INFO);
      image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      image_view_info.format = rnd.current_surface_format.format;
      image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      image_view_info.subresourceRange.baseArrayLayer = 0;
      image_view_info.subresourceRange.layerCount = 1;
      image_view_info.subresourceRange.baseMipLevel = 0;
      image_view_info.subresourceRange.levelCount = 1;
      for (auto cur = std::begin(rnd.swapchain_image_views); const auto& swapchain_image : rnd.swapchain_images)
      {
        image_view_info.image = swapchain_image;
        auto& image_view = *(cur++);
        if (auto result = vkCreateImageView(ctx.device, &image_view_info, nullptr, &image_view); result != VK_SUCCESS)
        {
          return result;
        }
      }
    }
    rnd.in_flight_images.resize(std::size(rnd.swapchain_images), VK_NULL_HANDLE);
    OBERON_POSTCONDITION(rnd.swapchain);
    OBERON_POSTCONDITION(std::size(rnd.swapchain_images) > 0);
    OBERON_POSTCONDITION(std::size(rnd.swapchain_images) == std::size(rnd.swapchain_image_views));
    OBERON_POSTCONDITION(std::size(rnd.in_flight_images) == std::size(rnd.swapchain_images));
    return 0;
  }

namespace {

  iresult create_main_renderpass(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateRenderPass);
    OBERON_PRECONDITION(ctx.device);
    auto renderpass_info = VkRenderPassCreateInfo{ };
    OBERON_INIT_VK_STRUCT(renderpass_info, RENDER_PASS_CREATE_INFO);
    auto color_attachment = VkAttachmentDescription{ };
    std::memset(&color_attachment, 0, sizeof(VkAttachmentDescription));
    color_attachment.format = rnd.current_surface_format.format;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    renderpass_info.pAttachments = &color_attachment;
    renderpass_info.attachmentCount = 1;
    auto subpass_description = VkSubpassDescription{ };
    std::memset(&subpass_description, 0, sizeof(VkSubpassDescription));
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    auto color_attachment_ref = VkAttachmentReference{ };
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    subpass_description.pColorAttachments = &color_attachment_ref;
    subpass_description.colorAttachmentCount = 1;
    renderpass_info.pSubpasses = &subpass_description;
    renderpass_info.subpassCount = 1;
    auto result = vkCreateRenderPass(ctx.device, &renderpass_info, nullptr, &rnd.main_renderpass);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    OBERON_POSTCONDITION(rnd.main_renderpass);
    return 0;
  }

}

  iresult create_vulkan_renderpasses(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    auto result = create_main_renderpass(ctx, rnd);
    if (OBERON_IS_IERROR(result))
    {
      goto ret;
    }
    // further render passes should follow this pattern here.
  ret:
    return result;
  }

  iresult create_vulkan_command_pools(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateCommandPool);
    OBERON_PRECONDITION(!rnd.graphics_transfer_command_pool);
    auto command_pool_info = VkCommandPoolCreateInfo{ };
    OBERON_INIT_VK_STRUCT(command_pool_info, COMMAND_POOL_CREATE_INFO);
    command_pool_info.queueFamilyIndex = ctx.graphics_transfer_queue_family;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    auto result = vkCreateCommandPool(ctx.device, &command_pool_info, nullptr, &rnd.graphics_transfer_command_pool);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    OBERON_POSTCONDITION(rnd.graphics_transfer_command_pool);
    return 0;
  }

  iresult destroy_vulkan_command_pools(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyCommandPool);
    if (rnd.graphics_transfer_command_pool)
    {
      vkDestroyCommandPool(ctx.device, rnd.graphics_transfer_command_pool, nullptr);
      rnd.graphics_transfer_command_pool = nullptr;
    }
    OBERON_POSTCONDITION(!rnd.graphics_transfer_command_pool);
    return 0;
  }

  iresult create_vulkan_command_buffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, AllocateCommandBuffers);
    OBERON_PRECONDITION(rnd.graphics_transfer_command_pool);
    OBERON_PRECONDITION(!std::size(rnd.graphics_transfer_command_buffers));
    rnd.graphics_transfer_command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    auto command_buffer_info = VkCommandBufferAllocateInfo{ };
    OBERON_INIT_VK_STRUCT(command_buffer_info, COMMAND_BUFFER_ALLOCATE_INFO);
    command_buffer_info.commandPool = rnd.graphics_transfer_command_pool;
    command_buffer_info.commandBufferCount = std::size(rnd.graphics_transfer_command_buffers);
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    auto result = vkAllocateCommandBuffers(ctx.device, &command_buffer_info,
                                           std::data(rnd.graphics_transfer_command_buffers));
    if (result != VK_SUCCESS)
    {
      return result;
    }
    OBERON_POSTCONDITION(std::size(rnd.graphics_transfer_command_buffers) == MAX_FRAMES_IN_FLIGHT);
    return 0;
  }

  iresult destroy_vulkan_command_buffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, FreeCommandBuffers);
    OBERON_PRECONDITION(rnd.graphics_transfer_command_pool);
    if (std::size(rnd.graphics_transfer_command_buffers))
    {
      vkFreeCommandBuffers(ctx.device, rnd.graphics_transfer_command_pool,
                           std::size(rnd.graphics_transfer_command_buffers),
                           std::data(rnd.graphics_transfer_command_buffers));
      rnd.graphics_transfer_command_buffers.resize(0);
    }
    OBERON_POSTCONDITION(!std::size(rnd.graphics_transfer_command_buffers));
    return 0;
  }

  iresult create_vulkan_framebuffers(const context_impl& ctx, const window_impl& win, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(rnd.swapchain);
    OBERON_PRECONDITION(std::size(rnd.swapchain_images) > 0);
    OBERON_PRECONDITION(std::size(rnd.swapchain_image_views) > 0);
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateFramebuffer);
    auto framebuffer_info = VkFramebufferCreateInfo{ };
    OBERON_INIT_VK_STRUCT(framebuffer_info, FRAMEBUFFER_CREATE_INFO);
    rnd.framebuffers.resize(std::size(rnd.swapchain_image_views));
    {
      auto current_color_view = std::begin(rnd.swapchain_image_views);
      auto attachments = std::array<VkImageView, 1>{ };
      auto& [ color_attachment ] = attachments;
      framebuffer_info.pAttachments = std::data(attachments);
      framebuffer_info.attachmentCount = std::size(attachments);
      framebuffer_info.renderPass = rnd.main_renderpass;
      framebuffer_info.layers = 1;
      framebuffer_info.width = rnd.current_swapchain_extent.width;
      framebuffer_info.height = rnd.current_swapchain_extent.height;
      auto result = VkResult{ };
      for (auto& framebuffer : rnd.framebuffers)
      {
        color_attachment = *(current_color_view++);
        result = vkCreateFramebuffer(ctx.device, &framebuffer_info, nullptr, &framebuffer);
        if (result != VK_SUCCESS)
        {
          return result;
        }
      }
    }
    OBERON_POSTCONDITION(std::size(rnd.framebuffers) > 0);
    OBERON_POSTCONDITION(std::size(rnd.framebuffers) == std::size(rnd.swapchain_image_views));
    return 0;
  }

  iresult create_vulkan_pipeline_cache(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreatePipelineCache);
    OBERON_PRECONDITION(!rnd.pipeline_cache);
    auto pipeline_cache_info = VkPipelineCacheCreateInfo{ };
    OBERON_INIT_VK_STRUCT(pipeline_cache_info, PIPELINE_CACHE_CREATE_INFO);
    auto result = vkCreatePipelineCache(ctx.device, &pipeline_cache_info, nullptr, &rnd.pipeline_cache);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    OBERON_POSTCONDITION(rnd.pipeline_cache);
    return 0;
  }

  iresult destroy_vulkan_pipeline_cache(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyPipelineCache);
    if (rnd.pipeline_cache)
    {
      vkDestroyPipelineCache(ctx.device, rnd.pipeline_cache, nullptr);
      rnd.pipeline_cache = nullptr;
    }
    OBERON_POSTCONDITION(!rnd.pipeline_cache);
    return 0;
  }

  iresult configure_test_frame_pipeline(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateShaderModule);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreatePipelineLayout);
    auto& config = rnd.graphics_pipeline_configs[static_cast<usize>(builtin_shader_name::test_frame)];
    // Begin GFX pipeline config
    OBERON_INIT_VK_STRUCT(config.graphics_pipeline_info, GRAPHICS_PIPELINE_CREATE_INFO);
    // Shader stages
    auto pipeline_shader_stage_info = VkPipelineShaderStageCreateInfo{ };
    OBERON_INIT_VK_STRUCT(pipeline_shader_stage_info, PIPELINE_SHADER_STAGE_CREATE_INFO);
    pipeline_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    auto module_info = VkShaderModuleCreateInfo{ };
    OBERON_INIT_VK_STRUCT(module_info, SHADER_MODULE_CREATE_INFO);
    OBERON_GET_VERTEX_BINARY(test_frame, module_info.pCode, module_info.codeSize);
    auto result = vkCreateShaderModule(ctx.device, &module_info, nullptr, &pipeline_shader_stage_info.module);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    pipeline_shader_stage_info.pName = "main";
    config.pipeline_stages.push_back(pipeline_shader_stage_info);
    pipeline_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    OBERON_GET_FRAGMENT_BINARY(test_frame, module_info.pCode, module_info.codeSize);
    result = vkCreateShaderModule(ctx.device, &module_info, nullptr, &pipeline_shader_stage_info.module);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    config.pipeline_stages.push_back(pipeline_shader_stage_info);
    config.graphics_pipeline_info.pStages = std::data(config.pipeline_stages);
    config.graphics_pipeline_info.stageCount = std::size(config.pipeline_stages);
    // Vertex Inputs
    OBERON_INIT_VK_STRUCT(config.vertex_input_state_info, PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
    config.graphics_pipeline_info.pVertexInputState = &config.vertex_input_state_info;
    // Input Assembly
    OBERON_INIT_VK_STRUCT(config.input_assembly_state_info, PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
    config.input_assembly_state_info.primitiveRestartEnable = false;
    config.input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config.graphics_pipeline_info.pInputAssemblyState = &config.input_assembly_state_info;
    // No Tessellation
    // Viewports
    OBERON_INIT_VK_STRUCT(config.viewport_state_info, PIPELINE_VIEWPORT_STATE_CREATE_INFO);
    auto viewport = VkViewport{ };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(rnd.current_swapchain_extent.width);
    viewport.height = static_cast<f32>(rnd.current_swapchain_extent.height);
    config.viewports.push_back(viewport);
    auto scissor = VkRect2D{ };
    scissor.offset = { 0, 0 };
    scissor.extent = rnd.current_swapchain_extent;
    config.scissors.push_back(scissor);
    config.viewport_state_info.pViewports = std::data(config.viewports);
    config.viewport_state_info.viewportCount = std::size(config.viewports);
    config.viewport_state_info.pScissors = std::data(config.scissors);
    config.viewport_state_info.scissorCount = std::size(config.scissors);
    config.graphics_pipeline_info.pViewportState = &config.viewport_state_info;
    // Rasterization
    OBERON_INIT_VK_STRUCT(config.rasterization_state_info, PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
    config.rasterization_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
    config.rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    config.rasterization_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    config.rasterization_state_info.lineWidth = 1.0f;
    config.graphics_pipeline_info.pRasterizationState = &config.rasterization_state_info;
    // Multisampling
    OBERON_INIT_VK_STRUCT(config.multisample_state_info, PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
    config.multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    config.graphics_pipeline_info.pMultisampleState = &config.multisample_state_info;
    // No Depth-Stencil
    // Color Blending
    OBERON_INIT_VK_STRUCT(config.color_blend_state_info, PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
    auto color_blend_attachment = VkPipelineColorBlendAttachmentState{ };
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = true;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    config.color_blend_attachments.push_back(color_blend_attachment);
    config.color_blend_state_info.pAttachments = std::data(config.color_blend_attachments);
    config.color_blend_state_info.attachmentCount = std::size(config.color_blend_attachments);
    config.graphics_pipeline_info.pColorBlendState = &config.color_blend_state_info;
    // No Dynamic States
    OBERON_INIT_VK_STRUCT(config.pipeline_layout_info, PIPELINE_LAYOUT_CREATE_INFO);
    result = vkCreatePipelineLayout(ctx.device, &config.pipeline_layout_info, nullptr,
                                    &config.graphics_pipeline_info.layout);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    // Final Configuration
    OBERON_POSTCONDITION(config.graphics_pipeline_info.layout);
    return 0;
  }

  iresult create_vulkan_graphics_pipelines(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateGraphicsPipelines);
    OBERON_PRECONDITION(rnd.pipeline_cache);
    auto configs = std::vector<VkGraphicsPipelineCreateInfo>(std::size(rnd.graphics_pipeline_configs));
    for (auto cur = std::begin(configs); auto& config : rnd.graphics_pipeline_configs)
    {
      if (config.graphics_pipeline_info.pViewportState)
      {
        config.viewports[0].width = rnd.current_swapchain_extent.width;
        config.viewports[0].height = rnd.current_swapchain_extent.height;
        config.scissors[0].extent = rnd.current_swapchain_extent;
      }
      config.graphics_pipeline_info.renderPass = rnd.main_renderpass;
      config.graphics_pipeline_info.subpass = 0;
      *(cur++) = config.graphics_pipeline_info;
    }
    auto result = vkCreateGraphicsPipelines(ctx.device, rnd.pipeline_cache, std::size(configs), std::data(configs),
                                            nullptr, std::data(rnd.graphics_pipelines));
    if (result != VK_SUCCESS)
    {
      return result;
    }
    return 0;
  }

  iresult destroy_vulkan_graphics_pipelines(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyPipeline);
    for (const auto& pipeline : rnd.graphics_pipelines)
    {
      vkDestroyPipeline(ctx.device, pipeline, nullptr);
    }
    return 0;
  }

  iresult release_graphics_pipeline_configurations(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyShaderModule);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyPipelineLayout);
    for (auto& config : rnd.graphics_pipeline_configs)
    {
      for (auto& pipeline_stage : config.pipeline_stages)
      {
        vkDestroyShaderModule(ctx.device, pipeline_stage.module, nullptr);
      }
      vkDestroyPipelineLayout(ctx.device, config.graphics_pipeline_info.layout, nullptr);
    }
    rnd.graphics_pipeline_configs.clear();
    OBERON_POSTCONDITION(!std::size(rnd.graphics_pipeline_configs));
    return 0;
  }

  iresult destroy_vulkan_framebuffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyFramebuffer);
    for (const auto& framebuffer : rnd.framebuffers)
    {
      if (framebuffer)
      {
        vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
      }
    }
    rnd.framebuffers.resize(0);
    OBERON_POSTCONDITION(std::size(rnd.framebuffers) == 0);
    return 0;
  }

  iresult destroy_vulkan_renderpasses(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyRenderPass);
    if (rnd.main_renderpass)
    {
      vkDestroyRenderPass(ctx.device, rnd.main_renderpass, nullptr);
      rnd.main_renderpass = nullptr;
    }
    return 0;
  }

  iresult destroy_vulkan_swapchain(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    if (!rnd.swapchain)
    {
      return 0;
    }
    OBERON_ASSERT(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroySwapchainKHR);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyImageView);
    for (const auto& swapchain_image_view : rnd.swapchain_image_views)
    {
      vkDestroyImageView(ctx.device, swapchain_image_view, nullptr);
    }
    vkDestroySwapchainKHR(ctx.device, rnd.swapchain, nullptr);
    rnd.swapchain_images.resize(0);
    rnd.swapchain_image_views.resize(0);
    rnd.in_flight_images.resize(0);
    rnd.swapchain = nullptr;
    OBERON_POSTCONDITION(!rnd.swapchain);
    return 0;
  }

  iresult begin_vulkan_command_buffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, BeginCommandBuffer);
    OBERON_DECLARE_VK_PFN(ctx.dl, ResetCommandBuffer);
    OBERON_PRECONDITION(std::size(rnd.graphics_transfer_command_buffers));
    auto& command_buffer = rnd.graphics_transfer_command_buffers[rnd.frame_index];
    auto result = vkResetCommandBuffer(command_buffer, 0);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    auto buffer_begin_info = VkCommandBufferBeginInfo{ };
    OBERON_INIT_VK_STRUCT(buffer_begin_info, COMMAND_BUFFER_BEGIN_INFO);
    //for (auto& command_buffer : rnd.graphics_transfer_command_buffers)
    {
      result = vkBeginCommandBuffer(command_buffer, &buffer_begin_info);
      if (result != VK_SUCCESS)
      {
        return result;
      }
    }
    return 0;
  }

  iresult end_vulkan_command_buffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, EndCommandBuffer);
    OBERON_PRECONDITION(std::size(rnd.graphics_transfer_command_buffers));
    auto result = VK_SUCCESS;
    //for (auto& command_buffer : rnd.graphics_transfer_command_buffers)
    auto& command_buffer = rnd.graphics_transfer_command_buffers[rnd.frame_index];
    {
      result = vkEndCommandBuffer(command_buffer);
      if (result != VK_SUCCESS)
      {
        return result;
      }
    }
    return 0;
  }

  iresult begin_main_render_pass(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, CmdBeginRenderPass);
    OBERON_PRECONDITION(std::size(rnd.graphics_transfer_command_buffers));
    auto render_pass_info = VkRenderPassBeginInfo{ };
    OBERON_INIT_VK_STRUCT(render_pass_info, RENDER_PASS_BEGIN_INFO);
    render_pass_info.renderPass = rnd.main_renderpass;
    render_pass_info.renderArea = { { 0, 0 }, rnd.current_swapchain_extent };
    auto clear_value = VkClearValue{ };
    std::fill(std::begin(clear_value.color.float32), std::end(clear_value.color.float32) - 1, 0.2f);
    clear_value.color.float32[3] = 1.0f;
    render_pass_info.pClearValues = &clear_value;
    render_pass_info.clearValueCount = 1;
    render_pass_info.framebuffer = rnd.framebuffers[rnd.acquired_image_index];
    auto& command_buffer = rnd.graphics_transfer_command_buffers[rnd.frame_index];
    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    return 0;
  }

  iresult end_main_render_pass(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, CmdEndRenderPass);
    OBERON_PRECONDITION(std::size(rnd.graphics_transfer_command_buffers));
    vkCmdEndRenderPass(rnd.graphics_transfer_command_buffers[rnd.frame_index]);
    return 0;
  }

  iresult draw_test_frame(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, CmdBindPipeline);
    OBERON_DECLARE_VK_PFN(ctx.dl, CmdDraw);
    OBERON_PRECONDITION(rnd.acquired_image_index < -1U);
    OBERON_PRECONDITION(std::size(rnd.graphics_transfer_command_buffers));
    auto& command_buffer = rnd.graphics_transfer_command_buffers[rnd.frame_index];
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      rnd.graphics_pipelines[static_cast<usize>(builtin_shader_name::test_frame)]);
    vkCmdDraw(command_buffer, 3, 1, 0, 0);
    return 0;
  }

  iresult create_vulkan_synchronization_objects(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateSemaphore);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateFence);
    OBERON_PRECONDITION(!std::size(rnd.image_available_semaphores));
    OBERON_PRECONDITION(!std::size(rnd.render_complete_semaphores));
    OBERON_PRECONDITION(!std::size(rnd.in_flight_fences));
    rnd.image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    rnd.render_complete_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    rnd.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    auto semaphore_info = VkSemaphoreCreateInfo{ };
    OBERON_INIT_VK_STRUCT(semaphore_info, SEMAPHORE_CREATE_INFO);
    auto fence_info = VkFenceCreateInfo{ };
    OBERON_INIT_VK_STRUCT(fence_info, FENCE_CREATE_INFO);
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    auto result = VK_SUCCESS;
    for (auto i = usize{ 0 }; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
      result = vkCreateSemaphore(ctx.device, &semaphore_info, nullptr, &rnd.image_available_semaphores[i]);
      if (result != VK_SUCCESS)
      {
        return result;
      }
      result = vkCreateSemaphore(ctx.device, &semaphore_info, nullptr, &rnd.render_complete_semaphores[i]);
      if (result != VK_SUCCESS)
      {
        return result;
      }
      result = vkCreateFence(ctx.device, &fence_info, nullptr, &rnd.in_flight_fences[i]);
      if (result != VK_SUCCESS)
      {
        return result;
      }
    }
    OBERON_POSTCONDITION(std::size(rnd.image_available_semaphores) == std::size(rnd.render_complete_semaphores));
    OBERON_POSTCONDITION(std::size(rnd.in_flight_fences) == std::size(rnd.image_available_semaphores));
    return 0;
  }

  iresult destroy_vulkan_synchronization_objects(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroySemaphore);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyFence);
    OBERON_PRECONDITION(std::size(rnd.image_available_semaphores) == std::size(rnd.render_complete_semaphores));
    OBERON_PRECONDITION(std::size(rnd.in_flight_fences) == std::size(rnd.image_available_semaphores));
    for (auto i = usize{ 0 }; i < std::size(rnd.image_available_semaphores); ++i)
    {
      vkDestroySemaphore(ctx.device, rnd.image_available_semaphores[i], nullptr);
      vkDestroySemaphore(ctx.device, rnd.render_complete_semaphores[i], nullptr);
      vkDestroyFence(ctx.device, rnd.in_flight_fences[i], nullptr);
    }
    rnd.image_available_semaphores.resize(0);
    rnd.render_complete_semaphores.resize(0);
    rnd.in_flight_fences.resize(0);
    return 0;
  }

  iresult acquire_frame(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_PRECONDITION(rnd.swapchain);
    OBERON_DECLARE_VK_PFN(ctx.dl, AcquireNextImageKHR);
    OBERON_DECLARE_VK_PFN(ctx.dl, WaitForFences);
    auto result = vkWaitForFences(ctx.device, 1, &rnd.in_flight_fences[rnd.frame_index], true, -1ULL);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    result = vkAcquireNextImageKHR(ctx.device, rnd.swapchain, -1ULL,
                                   rnd.image_available_semaphores[rnd.frame_index], VK_NULL_HANDLE,
                                   &rnd.acquired_image_index);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    auto& fence = rnd.in_flight_images[rnd.acquired_image_index];
    if (fence)
    {
      result = vkWaitForFences(ctx.device, 1, &fence, true, -1ULL);
      if (result != VK_SUCCESS)
      {
        return result;
      }
    }
    rnd.in_flight_images[rnd.acquired_image_index] = VK_NULL_HANDLE;
    OBERON_POSTCONDITION(rnd.acquired_image_index < -1U);
    return 0;
  }

  iresult submit_frame(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, QueueSubmit);
    OBERON_DECLARE_VK_PFN(ctx.dl, QueuePresentKHR);
    OBERON_DECLARE_VK_PFN(ctx.dl, ResetFences);
    OBERON_PRECONDITION(rnd.acquired_image_index < -1U);
    auto submit_info = VkSubmitInfo{ };
    OBERON_INIT_VK_STRUCT(submit_info, SUBMIT_INFO);
    submit_info.pWaitSemaphores = &rnd.image_available_semaphores[rnd.frame_index];
    submit_info.waitSemaphoreCount = 1;
    auto wait_stages = VkPipelineStageFlags{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = &wait_stages;
    submit_info.pCommandBuffers = &rnd.graphics_transfer_command_buffers[rnd.frame_index];
    submit_info.commandBufferCount = 1;
    submit_info.pSignalSemaphores = &rnd.render_complete_semaphores[rnd.frame_index];
    submit_info.signalSemaphoreCount = 1;
    auto result = vkResetFences(ctx.device, 1, &rnd.in_flight_fences[rnd.frame_index]);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    result = vkQueueSubmit(ctx.graphics_transfer_queue, 1, &submit_info, rnd.in_flight_fences[rnd.frame_index]);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    auto present_info = VkPresentInfoKHR{ };
    OBERON_INIT_VK_STRUCT(present_info, PRESENT_INFO_KHR);
    present_info.pImageIndices = &rnd.acquired_image_index;
    present_info.pSwapchains = &rnd.swapchain;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &rnd.render_complete_semaphores[rnd.frame_index];
    present_info.waitSemaphoreCount = 1;
    result = vkQueuePresentKHR(ctx.presentation_queue, &present_info);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    rnd.in_flight_images[rnd.acquired_image_index] = rnd.in_flight_fences[rnd.frame_index];
    rnd.acquired_image_index = -1U;
    rnd.frame_index = (rnd.frame_index + 1) & (MAX_FRAMES_IN_FLIGHT - 1); // frame_index % MAX_FRAMES_IN_FLIGHT
    return 0;
  }
}

  void renderer_3d::v_dispose() noexcept {
    auto& rnd = reference_cast<detail::renderer_3d_impl>(implementation());
    auto& ctx = reference_cast<detail::context_impl>(parent().parent().implementation());
    detail::wait_for_device_idle(ctx);
    detail::destroy_vulkan_synchronization_objects(ctx, rnd);
    detail::destroy_vulkan_graphics_pipelines(ctx, rnd);
    detail::release_graphics_pipeline_configurations(ctx, rnd);
    detail::destroy_vulkan_pipeline_cache(ctx, rnd);
    detail::destroy_vulkan_command_buffers(ctx, rnd);
    detail::destroy_vulkan_framebuffers(ctx, rnd);
    detail::destroy_vulkan_command_pools(ctx, rnd);
    detail::destroy_vulkan_renderpasses(ctx, rnd);
    detail::destroy_vulkan_swapchain(ctx, rnd);
  }

  renderer_3d::renderer_3d(const window& win) : object{ new detail::renderer_3d_impl{ }, &win } {
    auto& rnd = reference_cast<detail::renderer_3d_impl>(implementation());
    auto& win_impl = reference_cast<detail::window_impl>(parent().implementation());
    auto& ctx = reference_cast<detail::context_impl>(parent().parent().implementation());
    rnd.graphics_pipeline_configs.resize(detail::BUILTIN_SHADER_COUNT);
    rnd.graphics_pipelines.resize(detail::BUILTIN_SHADER_COUNT);
    detail::retrieve_vulkan_surface_info(ctx, win_impl, rnd);
    if (OBERON_IS_IERROR(detail::create_vulkan_swapchain(ctx, win_impl, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan swapchain." };
    }
    //TODO create depth/stencil images with some kind of allocator here.
    if (OBERON_IS_IERROR(detail::create_vulkan_renderpasses(ctx, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan render passes." };
    }
    if (OBERON_IS_IERROR(detail::create_vulkan_command_pools(ctx, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan command pools." };
    }
    if (OBERON_IS_IERROR(detail::create_vulkan_framebuffers(ctx, win_impl, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan framebuffers." };
    }
    if (OBERON_IS_IERROR(detail::create_vulkan_command_buffers(ctx, rnd)))
    {
      throw fatal_error{ "Failed to allocate Vulkan command buffers." };
    }
    if (OBERON_IS_IERROR(detail::create_vulkan_pipeline_cache(ctx, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan pipeline cache." };
    }
    if (OBERON_IS_IERROR(detail::configure_test_frame_pipeline(ctx, rnd)))
    {
      throw fatal_error{ "Failed to configure test_frame pipeline." };
    }
    if (OBERON_IS_IERROR(detail::create_vulkan_graphics_pipelines(ctx, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan graphics pipelines." };
    }
    if (OBERON_IS_IERROR(detail::create_vulkan_synchronization_objects(ctx, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan semaphores." };
    }
  }

  renderer_3d::~renderer_3d() noexcept {
    dispose();
  }


  renderer_3d& renderer_3d::begin_frame() {
    auto& rnd = reference_cast<detail::renderer_3d_impl>(implementation());
    auto& ctx = reference_cast<detail::context_impl>(parent().parent().implementation());
    if (OBERON_IS_IERROR(detail::acquire_frame(ctx, rnd)))
    {
      throw fatal_error{ "Failed to acquire next image for drawing." };
    }
    if (OBERON_IS_IERROR(detail::begin_vulkan_command_buffers(ctx, rnd)))
    {
      throw fatal_error{ "Failed to begin Vulkan command buffer recording." };
    }
    detail::begin_main_render_pass(ctx, rnd);
    return *this;
  }

  renderer_3d& renderer_3d::end_frame() {
    auto& rnd = reference_cast<detail::renderer_3d_impl>(implementation());
    auto& ctx = reference_cast<detail::context_impl>(parent().parent().implementation());
    detail::end_main_render_pass(ctx, rnd);
    if (OBERON_IS_IERROR(detail::end_vulkan_command_buffers(ctx, rnd)))
    {
      throw fatal_error{ "Failed to end Vulkan command buffer recording." };
    }
    auto result = detail::submit_frame(ctx, rnd);
    switch (result)
    {
    case VK_ERROR_OUT_OF_DATE_KHR: // Rebuild swapchain
    case VK_SUBOPTIMAL_KHR:
      rnd.should_rebuild = true;
    case 0:
      break;
    default:
      throw fatal_error{ "Failed to submit image for presentation." };
    }
    return *this;
  }

  renderer_3d& renderer_3d::draw_test_frame() {
    auto& rnd = reference_cast<detail::renderer_3d_impl>(implementation());
    auto& ctx = reference_cast<detail::context_impl>(parent().parent().implementation());
    detail::draw_test_frame(ctx, rnd);
    return *this;
  }

  bool renderer_3d::should_rebuild() const {
    auto& rnd = reference_cast<detail::renderer_3d_impl>(implementation());
    return rnd.should_rebuild;
  }

  renderer_3d& renderer_3d::rebuild() {
    auto& rnd = reference_cast<detail::renderer_3d_impl>(implementation());
    auto& win = reference_cast<detail::window_impl>(parent().implementation());
    auto& ctx = reference_cast<detail::context_impl>(parent().parent().implementation());
    detail::wait_for_device_idle(ctx);
    detail::destroy_vulkan_graphics_pipelines(ctx, rnd);
    detail::destroy_vulkan_framebuffers(ctx, rnd);
    detail::destroy_vulkan_renderpasses(ctx, rnd);
    detail::destroy_vulkan_swapchain(ctx, rnd);
    detail::retrieve_vulkan_surface_info(ctx, win, rnd);
    if (OBERON_IS_IERROR(detail::create_vulkan_swapchain(ctx, win, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan swapchain." };
    }
    //TODO create depth/stencil images with some kind of allocator here.
    if (OBERON_IS_IERROR(detail::create_vulkan_renderpasses(ctx, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan render passes." };
    }
    if (OBERON_IS_IERROR(detail::create_vulkan_framebuffers(ctx, win, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan framebuffers." };
    }
    if (OBERON_IS_IERROR(detail::create_vulkan_graphics_pipelines(ctx, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan graphics pipelines." };
    }
    rnd.should_rebuild = false; // :-( don't forget to reset this flag!
    return *this;
  }
}
