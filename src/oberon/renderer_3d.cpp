#include "oberon/detail/renderer_3d_impl.hpp"

#include <cstring>

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
    OBERON_PRECONDITION(ctx.vkft.vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    OBERON_PRECONDITION(ctx.vkft.vkGetPhysicalDeviceSurfaceFormatsKHR);
    OBERON_PRECONDITION(ctx.vkft.vkGetPhysicalDeviceSurfacePresentModesKHR);
    OBERON_PRECONDITION(win.surface);
    auto vkGetPhysicalDeviceSurfaceCapabilitiesKHR = ctx.vkft.vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    auto vkGetPhysicalDeviceSurfaceFormatsKHR = ctx.vkft.vkGetPhysicalDeviceSurfaceFormatsKHR;
    auto vkGetPhysicalDeviceSurfacePresentModesKHR = ctx.vkft.vkGetPhysicalDeviceSurfacePresentModesKHR;

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
    OBERON_PRECONDITION(ctx.vkft.vkGetPhysicalDeviceSurfaceSupportKHR);
    OBERON_PRECONDITION(ctx.vkft.vkCreateSwapchainKHR);
    OBERON_PRECONDITION(ctx.vkft.vkGetSwapchainImagesKHR);
    OBERON_PRECONDITION(ctx.vkft.vkCreateImageView);

    auto vkGetPhysicalDeviceSurfaceSupportKHR = ctx.vkft.vkGetPhysicalDeviceSurfaceSupportKHR;
    auto vkCreateSwapchainKHR = ctx.vkft.vkCreateSwapchainKHR;
    auto vkGetSwapchainImagesKHR = ctx.vkft.vkGetSwapchainImagesKHR;
    auto vkCreateImageView = ctx.vkft.vkCreateImageView;

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
      swapchain_info.imageExtent = rnd.surface_capabilities.currentExtent;
    }
    else
    {
      auto& capabilities = rnd.surface_capabilities;
      auto actual_extent = VkExtent2D{
        static_cast<u32>(win.bounds.size.width), static_cast<u32>(win.bounds.size.height)
      };
      swapchain_info.imageExtent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      swapchain_info.imageExtent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
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
    OBERON_POSTCONDITION(rnd.swapchain);
    OBERON_POSTCONDITION(std::size(rnd.swapchain_images) > 0);
    OBERON_POSTCONDITION(std::size(rnd.swapchain_image_views) > 0);
    OBERON_POSTCONDITION(std::size(rnd.swapchain_images) == std::size(rnd.swapchain_image_views));
    return 0;
  }

namespace {

  iresult create_main_renderpass(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.vkft.vkCreateRenderPass);
    OBERON_PRECONDITION(ctx.device);
    auto vkCreateRenderPass = ctx.vkft.vkCreateRenderPass;
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

  iresult create_vulkan_framebuffers(const context_impl& ctx, const window_impl& win, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(rnd.swapchain);
    OBERON_PRECONDITION(std::size(rnd.swapchain_images) > 0);
    OBERON_PRECONDITION(std::size(rnd.swapchain_image_views) > 0);
    OBERON_PRECONDITION(ctx.device);
    OBERON_PRECONDITION(ctx.vkft.vkCreateFramebuffer);
    auto vkCreateFramebuffer = ctx.vkft.vkCreateFramebuffer;
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
      framebuffer_info.width = win.bounds.size.width;
      framebuffer_info.height = win.bounds.size.height;
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

  iresult destroy_vulkan_framebuffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_PRECONDITION(ctx.vkft.vkDestroyFramebuffer);
    auto vkDestroyFramebuffer = ctx.vkft.vkDestroyFramebuffer;
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
    OBERON_PRECONDITION(ctx.vkft.vkDestroyRenderPass);
    auto vkDestroyRenderPass = ctx.vkft.vkDestroyRenderPass;
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
    OBERON_ASSERT(ctx.vkft.vkDestroySwapchainKHR);
    OBERON_ASSERT(ctx.vkft.vkDestroyImageView);
    auto vkDestroyImageView = ctx.vkft.vkDestroyImageView;
    auto vkDestroySwapchainKHR = ctx.vkft.vkDestroySwapchainKHR;

    for (const auto& swapchain_image_view : rnd.swapchain_image_views)
    {
      vkDestroyImageView(ctx.device, swapchain_image_view, nullptr);
    }
    vkDestroySwapchainKHR(ctx.device, rnd.swapchain, nullptr);
    rnd.swapchain_images.resize(0);
    rnd.swapchain_image_views.resize(0);
    rnd.swapchain = nullptr;
    OBERON_POSTCONDITION(!rnd.swapchain);
    OBERON_POSTCONDITION(std::size(rnd.swapchain_images) == 0);
    OBERON_POSTCONDITION(std::size(rnd.swapchain_image_views) == 0);
    return 0;
  }
}

  void renderer_3d::v_dispose() noexcept {
    auto& rnd = reference_cast<detail::renderer_3d_impl>(implementation());
    auto& ctx = reference_cast<detail::context_impl>(parent().parent().implementation());
    detail::destroy_vulkan_framebuffers(ctx, rnd);
    detail::destroy_vulkan_renderpasses(ctx, rnd);
    detail::destroy_vulkan_swapchain(ctx, rnd);
  }

  renderer_3d::renderer_3d(const window& win) : object{ new detail::renderer_3d_impl{ }, &win } {
    auto& rnd = reference_cast<detail::renderer_3d_impl>(implementation());
    auto& win_impl = reference_cast<detail::window_impl>(parent().implementation());
    auto& ctx = reference_cast<detail::context_impl>(parent().parent().implementation());
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
    if (OBERON_IS_IERROR(detail::create_vulkan_framebuffers(ctx, win_impl, rnd)))
    {
      throw fatal_error{ "Failed to create Vulkan framebuffers." };
    }
  }

  renderer_3d::~renderer_3d() noexcept {
    dispose();
  }

}
