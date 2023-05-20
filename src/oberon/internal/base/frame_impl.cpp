#include "oberon/internal/base/frame_impl.hpp"

#include "oberon/errors.hpp"
#include "oberon/utility.hpp"
#include "oberon/mesh.hpp"
#include "oberon/camera.hpp"

#include "oberon/internal/base/mesh_impl.hpp"

#include "oberon/internal/base/graphics_device_impl.hpp"

#define VK_STRUCT(name) OBERON_INTERNAL_BASE_VK_STRUCT(name)
#define VK_DECLARE_PFN(dl, cmd) OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, cmd)
#define VK_SUCCEEDS(exp) OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp)

namespace oberon::internal::base {

  frame_impl::frame_impl(graphics_device_impl& device, const VkFormat color_format,
                         const VkFormat depth_stencil_format, const VkExtent3D& extent,
                         const VkSampleCountFlagBits samples,
                         std::array<VkPipelineLayout, PIPELINE_COUNT>& layouts,
                         std::array<VkPipeline, PIPELINE_COUNT>& pipelines) :
  m_parent{ &device },
  m_layouts{ &layouts },
  m_pipelines{ &pipelines } {
    auto fence_info = VkFenceCreateInfo{ };
    fence_info.sType = VK_STRUCT(FENCE_CREATE_INFO);
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreateFence);
    for (auto& fence : m_fences)
    {
      VK_SUCCEEDS(vkCreateFence(m_parent->device_handle(), &fence_info, nullptr, &fence));
    }
    auto semaphore_info = VkSemaphoreCreateInfo{ };
    semaphore_info.sType = VK_STRUCT(SEMAPHORE_CREATE_INFO);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreateSemaphore);
    for (auto cur = m_semaphores.begin() + 1; cur != m_semaphores.end(); ++cur)
    {
      auto& semaphore = *cur;
      VK_SUCCEEDS(vkCreateSemaphore(m_parent->device_handle(), &semaphore_info, nullptr, &semaphore));
    }
    auto command_pool_info = VkCommandPoolCreateInfo{ };
    command_pool_info.sType = VK_STRUCT(COMMAND_POOL_CREATE_INFO);
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    command_pool_info.queueFamilyIndex = m_parent->queue_family();
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreateCommandPool);
    VK_SUCCEEDS(vkCreateCommandPool(m_parent->device_handle(), &command_pool_info, nullptr, &m_command_pool));
    auto command_buffer_info = VkCommandBufferAllocateInfo{ };
    command_buffer_info.sType = VK_STRUCT(COMMAND_BUFFER_ALLOCATE_INFO);
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandPool = m_command_pool;
    command_buffer_info.commandBufferCount = m_command_buffers.size();
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkAllocateCommandBuffers);
    VK_SUCCEEDS(vkAllocateCommandBuffers(m_parent->device_handle(), &command_buffer_info, m_command_buffers.data()));
    create_images(color_format, depth_stencil_format, extent, samples);
  }

  frame_impl::~frame_impl() noexcept {
    wait_for_availability();
    destroy_images();
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkFreeCommandBuffers);
    vkFreeCommandBuffers(m_parent->device_handle(), m_command_pool, m_command_buffers.size(),
                         m_command_buffers.data());
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkDestroyCommandPool);
    vkDestroyCommandPool(m_parent->device_handle(), m_command_pool, nullptr);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkDestroySemaphore);
    for (auto cur = m_semaphores.begin() + 1; cur != m_semaphores.end(); ++cur)
    {
      vkDestroySemaphore(m_parent->device_handle(), *cur, nullptr);
    }
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkDestroyFence);
    for (auto& fence : m_fences)
    {
      vkDestroyFence(m_parent->device_handle(), fence, nullptr);
    }
  }

  void frame_impl::create_images(const VkFormat color_format, const VkFormat depth_stencil_format,
                                 const VkExtent3D& extent, const VkSampleCountFlagBits samples) {
    auto image_info = VkImageCreateInfo{ };
    image_info.sType = VK_STRUCT(IMAGE_CREATE_INFO);
    image_info.extent = extent;
    image_info.format = color_format;
    image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.samples = samples;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.queueFamilyIndexCount = 1;
    const auto family = m_parent->queue_family();
    image_info.pQueueFamilyIndices = &family;
    auto allocation_info = VmaAllocationCreateInfo{ };
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    const auto image_size = extent.width * extent.height;
    if (image_size >= (1280 * 720))
    {
      allocation_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    }
    allocation_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    allocation_info.priority = 1.0f;
    VK_SUCCEEDS(vmaCreateImage(m_parent->allocator(), &image_info, &allocation_info, &m_color_attachment,
                               &m_color_allocation, nullptr));
    auto image_view_info = VkImageViewCreateInfo{ };
    image_view_info.sType = VK_STRUCT(IMAGE_VIEW_CREATE_INFO);
    image_view_info.format = color_format;
    image_view_info.image = m_color_attachment;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.layerCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseMipLevel = 0;
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreateImageView);
    VK_SUCCEEDS(vkCreateImageView(m_parent->device_handle(), &image_view_info, nullptr, &m_color_view));
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    VK_SUCCEEDS(vmaCreateImage(m_parent->allocator(), &image_info, &allocation_info, &m_resolve_attachment,
                               &m_resolve_allocation, nullptr));
    image_view_info.image = m_resolve_attachment;
    VK_SUCCEEDS(vkCreateImageView(m_parent->device_handle(), &image_view_info, nullptr, &m_resolve_view));
    m_color_attachment_info.sType = VK_STRUCT(RENDERING_ATTACHMENT_INFO);
    m_color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    m_color_attachment_info.clearValue.color = { { to_linear_color(0.2f), to_linear_color(0.2f),
                                                   to_linear_color(0.2f) } };
    m_color_attachment_info.imageView = m_color_view;
    m_color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    m_color_attachment_info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    m_color_attachment_info.resolveImageView = m_resolve_view;
    m_color_attachment_info.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_info.format = depth_stencil_format;
    image_info.samples = samples;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VK_SUCCEEDS(vmaCreateImage(m_parent->allocator(), &image_info, &allocation_info, &m_depth_stencil_attachment,
                               &m_depth_stencil_allocation, nullptr));
    image_view_info.format = depth_stencil_format;
    image_view_info.image = m_depth_stencil_attachment;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    VK_SUCCEEDS(vkCreateImageView(m_parent->device_handle(), &image_view_info, nullptr, &m_depth_stencil_view));
    m_depth_stencil_attachment_info.sType = VK_STRUCT(RENDERING_ATTACHMENT_INFO);
    m_depth_stencil_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_depth_stencil_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    m_depth_stencil_attachment_info.clearValue.depthStencil = { 1.0f, 0 };
    m_depth_stencil_attachment_info.imageView = m_depth_stencil_view;
    m_depth_stencil_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    m_current_render.sType = VK_STRUCT(RENDERING_INFO);
    m_current_render.colorAttachmentCount = 1;
    m_current_render.pColorAttachments = &m_color_attachment_info;
    m_current_render.pDepthAttachment = &m_depth_stencil_attachment_info;
    m_current_render.pStencilAttachment = &m_depth_stencil_attachment_info;
    m_current_render.viewMask = 0;
    m_current_render.layerCount = 1;
    m_current_render.renderArea = { { 0, 0 }, { extent.width, extent.height } };
    m_current_color_format = color_format;
    m_current_depth_stencil_format = depth_stencil_format;
    m_current_extent = extent;
  }

  void frame_impl::destroy_images() {
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkDestroyImageView);
    vkDestroyImageView(m_parent->device_handle(), m_depth_stencil_view, nullptr);
    vkDestroyImageView(m_parent->device_handle(), m_resolve_view, nullptr);
    vkDestroyImageView(m_parent->device_handle(), m_color_view, nullptr);
    vmaDestroyImage(m_parent->allocator(), m_depth_stencil_attachment, m_depth_stencil_allocation);
    vmaDestroyImage(m_parent->allocator(), m_resolve_attachment, m_resolve_allocation);
    vmaDestroyImage(m_parent->allocator(), m_color_attachment, m_color_allocation);
  }

  void frame_impl::wait_for_availability() {
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkWaitForFences);
    VK_SUCCEEDS(vkWaitForFences(m_parent->device_handle(), m_fences.size(), m_fences.data(), true, VK_FOREVER));
  }

  void frame_impl::make_unavailable() {
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkResetFences);
    VK_SUCCEEDS(vkResetFences(m_parent->device_handle(), m_fences.size(), m_fences.data()));
  }

  void frame_impl::begin_rendering() {
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkResetCommandPool);
    VK_SUCCEEDS(vkResetCommandPool(m_parent->device_handle(), m_command_pool, 0));
    auto begin_info = VkCommandBufferBeginInfo{ };
    begin_info.sType = VK_STRUCT(COMMAND_BUFFER_BEGIN_INFO);
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkBeginCommandBuffer);
    for (auto& command_buffer : m_command_buffers)
    {
      VK_SUCCEEDS(vkBeginCommandBuffer(command_buffer, &begin_info));
    }
    auto image_memory_barriers = std::array<VkImageMemoryBarrier, 3>{ };
    {
      auto& color_barrier = image_memory_barriers[0];
      color_barrier.sType = VK_STRUCT(IMAGE_MEMORY_BARRIER);
      color_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      color_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      color_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color_barrier.srcQueueFamilyIndex = -1;
      color_barrier.dstQueueFamilyIndex = -1;
      color_barrier.image = m_color_attachment;
      color_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      color_barrier.subresourceRange.baseMipLevel = 0;
      color_barrier.subresourceRange.levelCount = std::numeric_limits<u32>::max();
      color_barrier.subresourceRange.baseArrayLayer = 0;
      color_barrier.subresourceRange.layerCount = std::numeric_limits<u32>::max();
    }
    {
      auto& depth_stencil_barrier = image_memory_barriers[1];
      depth_stencil_barrier.sType = VK_STRUCT(IMAGE_MEMORY_BARRIER);
      depth_stencil_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      depth_stencil_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      depth_stencil_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      depth_stencil_barrier.srcQueueFamilyIndex = -1;
      depth_stencil_barrier.dstQueueFamilyIndex = -1;
      depth_stencil_barrier.image = m_depth_stencil_attachment;
      depth_stencil_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      depth_stencil_barrier.subresourceRange.baseMipLevel = 0;
      depth_stencil_barrier.subresourceRange.levelCount = std::numeric_limits<u32>::max();
      depth_stencil_barrier.subresourceRange.baseArrayLayer = 0;
      depth_stencil_barrier.subresourceRange.layerCount = std::numeric_limits<u32>::max();
    }
    {
      auto& color_barrier = image_memory_barriers[2];
      color_barrier.sType = VK_STRUCT(IMAGE_MEMORY_BARRIER);
      color_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      color_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      color_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color_barrier.srcQueueFamilyIndex = -1;
      color_barrier.dstQueueFamilyIndex = -1;
      color_barrier.image = m_resolve_attachment;
      color_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      color_barrier.subresourceRange.baseMipLevel = 0;
      color_barrier.subresourceRange.levelCount = std::numeric_limits<u32>::max();
      color_barrier.subresourceRange.baseArrayLayer = 0;
      color_barrier.subresourceRange.layerCount = std::numeric_limits<u32>::max();
    }
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdPipelineBarrier);
    vkCmdPipelineBarrier(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                         0, 0, nullptr, 0, nullptr, image_memory_barriers.size(), image_memory_barriers.data());
    m_current_render.flags = VK_RENDERING_SUSPENDING_BIT;
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdBeginRendering);
    vkCmdBeginRendering(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], &m_current_render);
    auto viewport = VkViewport{ };
    viewport.x = m_current_render.renderArea.offset.x;
    viewport.y = m_current_render.renderArea.offset.y;
    viewport.width = m_current_extent.width;
    viewport.height = m_current_extent.height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = m_current_extent.depth;
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdSetViewport);
    vkCmdSetViewport(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], 0, 1, &viewport);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdSetScissor);
    vkCmdSetScissor(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], 0, 1, &m_current_render.renderArea);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdEndRendering);
    vkCmdEndRendering(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX]);
    m_current_render.flags |= VK_RENDERING_RESUMING_BIT;
  }

  void frame_impl::end_rendering(const VkImage target, const VkFormat format, const VkExtent3D& extent,
                                 const VkImageLayout layout, const VkSemaphore acquired) {
    m_current_render.flags = VK_RENDERING_RESUMING_BIT;
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdBeginRendering);
    vkCmdBeginRendering(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], &m_current_render);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdEndRendering);
    vkCmdEndRendering(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX]);
    auto image_memory_barriers = std::array<VkImageMemoryBarrier, 2>{ };
    {
      auto& src_barrier = image_memory_barriers[0];
      src_barrier.sType = VK_STRUCT(IMAGE_MEMORY_BARRIER);
      src_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      src_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      src_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      src_barrier.srcQueueFamilyIndex = -1;
      src_barrier.dstQueueFamilyIndex = -1;
      src_barrier.image = m_resolve_attachment;
      src_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      src_barrier.subresourceRange.baseMipLevel = 0;
      src_barrier.subresourceRange.levelCount = std::numeric_limits<u32>::max();
      src_barrier.subresourceRange.baseArrayLayer = 0;
      src_barrier.subresourceRange.layerCount = std::numeric_limits<u32>::max();
    }
    {
      auto& dst_barrier = image_memory_barriers[1];
      dst_barrier.sType = VK_STRUCT(IMAGE_MEMORY_BARRIER);
      dst_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      dst_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      dst_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      dst_barrier.srcQueueFamilyIndex = -1;
      dst_barrier.dstQueueFamilyIndex = -1;
      dst_barrier.image = target;
      dst_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      dst_barrier.subresourceRange.baseMipLevel = 0;
      dst_barrier.subresourceRange.levelCount = std::numeric_limits<u32>::max();
      dst_barrier.subresourceRange.baseArrayLayer = 0;
      dst_barrier.subresourceRange.layerCount = std::numeric_limits<u32>::max();
    }
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdPipelineBarrier);
    vkCmdPipelineBarrier(m_command_buffers[COPY_BLIT_COMMAND_BUFFER_INDEX], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, image_memory_barriers.size(),
                         image_memory_barriers.data());
    if (format == m_current_color_format && extent.width == m_current_extent.width &&
        extent.height == m_current_extent.height && extent.depth == m_current_extent.depth)
    {
      auto copy_region = VkImageCopy{ };
      copy_region.srcOffset = { 0, 0, 0 };
      copy_region.srcSubresource.layerCount = 1;
      copy_region.srcSubresource.baseArrayLayer = 0;
      copy_region.srcSubresource.mipLevel = 0;
      copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      copy_region.dstOffset = { 0, 0, 0 };
      copy_region.dstSubresource.layerCount = 1;
      copy_region.dstSubresource.baseArrayLayer = 0;
      copy_region.dstSubresource.mipLevel = 0;
      copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      copy_region.extent = extent;
      VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdCopyImage);
      vkCmdCopyImage(m_command_buffers[COPY_BLIT_COMMAND_BUFFER_INDEX], m_resolve_attachment,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                     &copy_region);
    }
    else
    {
      auto blit_region = VkImageBlit{ };
      blit_region.srcOffsets[0] = { 0, 0, 0 };
      blit_region.srcOffsets[1] = { static_cast<i32>(m_current_extent.width),
                                    static_cast<i32>(m_current_extent.height),
                                    static_cast<i32>(m_current_extent.depth) };
      blit_region.srcSubresource.layerCount = 1;
      blit_region.srcSubresource.baseArrayLayer = 0;
      blit_region.srcSubresource.mipLevel = 0;
      blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit_region.dstOffsets[0] = { 0, 0, 0 };
      blit_region.dstOffsets[1] = { static_cast<i32>(extent.width), static_cast<i32>(extent.height),
                                    static_cast<i32>(extent.depth) };
      blit_region.dstSubresource.layerCount = 1;
      blit_region.dstSubresource.baseArrayLayer = 0;
      blit_region.dstSubresource.mipLevel = 0;
      blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdBlitImage);
      vkCmdBlitImage(m_command_buffers[COPY_BLIT_COMMAND_BUFFER_INDEX], m_resolve_attachment,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                     &blit_region, VK_FILTER_NEAREST);
    }
    auto color_barrier = VkImageMemoryBarrier{ };
    color_barrier.sType = VK_STRUCT(IMAGE_MEMORY_BARRIER);
    color_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    color_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    color_barrier.newLayout = layout;
    color_barrier.srcQueueFamilyIndex = -1;
    color_barrier.dstQueueFamilyIndex = -1;
    color_barrier.image = target;
    color_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_barrier.subresourceRange.baseMipLevel = 0;
    color_barrier.subresourceRange.levelCount = std::numeric_limits<u32>::max();
    color_barrier.subresourceRange.baseArrayLayer = 0;
    color_barrier.subresourceRange.layerCount = std::numeric_limits<u32>::max();
    vkCmdPipelineBarrier(m_command_buffers[COPY_BLIT_COMMAND_BUFFER_INDEX], VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &color_barrier);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkEndCommandBuffer);
    for (auto& command_buffer : m_command_buffers)
    {
      VK_SUCCEEDS(vkEndCommandBuffer(command_buffer));
    }
    auto submit_infos = std::array<VkSubmitInfo, COMMAND_BUFFER_COUNT>{ };
    auto render_wait_stage = VkPipelineStageFlags{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    auto copy_blit_wait_stage = VkPipelineStageFlags{ VK_PIPELINE_STAGE_TRANSFER_BIT };
    {
      auto& submit_info = submit_infos[RENDER_COMMAND_BUFFER_INDEX];
      submit_info.sType = VK_STRUCT(SUBMIT_INFO);
      submit_info.pWaitDstStageMask = &render_wait_stage;
      submit_info.signalSemaphoreCount = 1;
      submit_info.pSignalSemaphores = &m_semaphores[RENDER_FINISHED_SEMAPHORE_INDEX];
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &m_command_buffers[RENDER_COMMAND_BUFFER_INDEX];
    }
    {
      auto& submit_info = submit_infos[COPY_BLIT_COMMAND_BUFFER_INDEX];
      submit_info.sType = VK_STRUCT(SUBMIT_INFO);
      submit_info.pWaitDstStageMask = &copy_blit_wait_stage;
      if (acquired)
      {
        m_semaphores[TARGET_ACQUIRED_SEMAPHORE_INDEX] = acquired;
        submit_info.waitSemaphoreCount = 2;
        submit_info.pWaitSemaphores = &m_semaphores[TARGET_ACQUIRED_SEMAPHORE_INDEX];
      }
      else
      {
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &m_semaphores[RENDER_FINISHED_SEMAPHORE_INDEX];
      }
      submit_info.signalSemaphoreCount = 1;
      submit_info.pSignalSemaphores = &m_semaphores[COPY_BLIT_FINISHED_SEMAPHORE_INDEX];
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &m_command_buffers[COPY_BLIT_COMMAND_BUFFER_INDEX];
    }
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkResetFences);
    VK_SUCCEEDS(vkResetFences(m_parent->device_handle(), m_fences.size(), m_fences.data()));
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkQueueSubmit);
    VK_SUCCEEDS(vkQueueSubmit(m_parent->queue(), submit_infos.size(), submit_infos.data(),
                              m_fences[SUBMISSION_COMPLETE_FENCE_INDEX]));
    m_semaphores[TARGET_ACQUIRED_SEMAPHORE_INDEX] = VK_NULL_HANDLE;
  }

  VkSemaphore frame_impl::copy_blit_finished_semaphore() const {
    return m_semaphores[COPY_BLIT_FINISHED_SEMAPHORE_INDEX];
  }

  void frame_impl::draw_test_image() {
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdBeginRendering);
    vkCmdBeginRendering(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], &m_current_render);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdBindPipeline);
    vkCmdBindPipeline(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      (*m_pipelines)[TEST_IMAGE_PIPELINE_INDEX]);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdDraw);
    vkCmdDraw(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], 3, 1, 0, 0);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdEndRendering);
    vkCmdEndRendering(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX]);
  }

  void frame_impl::draw(camera& c, mesh& m) {
    auto& impl = m.implementation();
    if (impl.is_dirty())
    {
      auto region = VkBufferCopy{ };
      region.srcOffset = 0;
      region.dstOffset = 0;
      region.size = impl.size() * impl.vertex_size();
      VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdCopyBuffer);
      vkCmdCopyBuffer(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], impl.staging_buffer(), impl.resident_buffer(),
                      1, &region);
      auto barrier = VkMemoryBarrier{ };
      barrier.sType = VK_STRUCT(MEMORY_BARRIER);
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
      VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdPipelineBarrier);
      vkCmdPipelineBarrier(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    }
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdBeginRendering);
    vkCmdBeginRendering(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], &m_current_render);
    switch (impl.type())
    {
    case vertex_type::position_color:
      {
        VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdBindPipeline);
        vkCmdBindPipeline(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], VK_PIPELINE_BIND_POINT_GRAPHICS,
                          (*m_pipelines)[UNLIT_PC_PIPELINE_INDEX]);
        VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdPushConstants);
        vkCmdPushConstants(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], (*m_layouts)[UNLIT_PC_PIPELINE_INDEX],
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 64, &impl.transform());
        vkCmdPushConstants(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], (*m_layouts)[UNLIT_PC_PIPELINE_INDEX],
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 64, 128, &c);
      }
      break;
    default:
      OBERON_CHECK_ERROR_MSG(false, 1, "Failed to draw due to an unsupported vertex type.");
    }
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdBindVertexBuffers);
    const auto buffer = impl.resident_buffer();
    const auto offset = VkDeviceSize{ 0 };
    vkCmdBindVertexBuffers(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], 0, 1, &buffer, &offset);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdDraw);
    vkCmdDraw(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX], impl.size(), 1, 0, 0);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCmdEndRendering);
    vkCmdEndRendering(m_command_buffers[RENDER_COMMAND_BUFFER_INDEX]);
  }
}
