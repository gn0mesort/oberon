#include "oberon/internal/base/renderer_impl.hpp"

#include <vector>

#include "oberon/renderer.hpp"
#include "oberon/frame.hpp"
#include "oberon/vertices.hpp"

#include "oberon/internal/base/graphics_device_impl.hpp"
#include "oberon/internal/base/window_impl.hpp"
#include "oberon/internal/base/frame_impl.hpp"

#include "test_image_vert_spv.hpp"
#include "test_image_frag_spv.hpp"
#include "unlit_pc_vert_spv.hpp"
#include "unlit_pc_frag_spv.hpp"

#define VK_STRUCT(name) OBERON_INTERNAL_BASE_VK_STRUCT(name)
#define VK_DECLARE_PFN(dl, cmd) OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, cmd)
#define VK_SUCCEEDS(exp) OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp)

namespace oberon::internal::base {

  void renderer_impl_dtor::operator()(ptr<renderer_impl> p) const noexcept {
    delete p;
  }

  renderer_impl::renderer_impl(graphics_device_impl& device, const extent_2d& resolution) :
  m_parent{ &device }, m_extent{ resolution.width, resolution.height, 1 } {
    constexpr auto color_features = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
                                    VK_FORMAT_FEATURE_BLIT_SRC_BIT;
    const auto color_formats = std::vector<VkFormat>{ VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB };
    m_color_format = m_parent->select_image_format(color_formats, VK_IMAGE_TILING_OPTIMAL, color_features);
    constexpr auto depth_stencil_features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    const auto depth_stencil_formats = std::vector<VkFormat>{ VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                              VK_FORMAT_D24_UNORM_S8_UINT,
                                                              VK_FORMAT_D16_UNORM_S8_UINT };
    m_depth_stencil_format = m_parent->select_image_format(depth_stencil_formats, VK_IMAGE_TILING_OPTIMAL,
                                                           depth_stencil_features);
    auto semaphore_info = VkSemaphoreCreateInfo{ };
    semaphore_info.sType = VK_STRUCT(SEMAPHORE_CREATE_INFO);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreateSemaphore);
    for (auto& semaphore : m_target_acquired_semaphores)
    {
      VK_SUCCEEDS(vkCreateSemaphore(m_parent->device_handle(), &semaphore_info, nullptr, &semaphore));
    }
    {
      auto rendering_info = VkPipelineRenderingCreateInfo{ };
      rendering_info.sType = VK_STRUCT(PIPELINE_RENDERING_CREATE_INFO);
      rendering_info.colorAttachmentCount = 1;
      rendering_info.pColorAttachmentFormats = &m_color_format;
      rendering_info.depthAttachmentFormat = m_depth_stencil_format;
      rendering_info.stencilAttachmentFormat = m_depth_stencil_format;
      create_test_image_pipeline(rendering_info);
      create_unlit_pc_pipeline(rendering_info);
    }
    for (auto& frame : m_frames)
    {
      frame = new frame_impl{ *m_parent, m_color_format, m_depth_stencil_format, m_extent, m_pipeline_layouts,
                              m_pipelines };
    }
  }
  renderer_impl::renderer_impl(graphics_device_impl& device, window_impl& win) :
  m_parent{ &device } {
    const auto swap_extent = win.swapchain_extent();
    m_extent = { swap_extent.width, swap_extent.height, 1 };
    constexpr auto color_features = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
                                    VK_FORMAT_FEATURE_BLIT_SRC_BIT;
    const auto color_formats = std::vector<VkFormat>{ win.surface_format(), VK_FORMAT_B8G8R8A8_SRGB,
                                                      VK_FORMAT_R8G8B8A8_SRGB };
    m_color_format = m_parent->select_image_format(color_formats, VK_IMAGE_TILING_OPTIMAL, color_features);
    constexpr auto depth_stencil_features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    const auto depth_stencil_formats = std::vector<VkFormat>{ VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                              VK_FORMAT_D24_UNORM_S8_UINT,
                                                              VK_FORMAT_D16_UNORM_S8_UINT };
    m_depth_stencil_format = m_parent->select_image_format(depth_stencil_formats, VK_IMAGE_TILING_OPTIMAL,
                                                           depth_stencil_features);
    auto semaphore_info = VkSemaphoreCreateInfo{ };
    semaphore_info.sType = VK_STRUCT(SEMAPHORE_CREATE_INFO);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreateSemaphore);
    for (auto& semaphore : m_target_acquired_semaphores)
    {
      VK_SUCCEEDS(vkCreateSemaphore(m_parent->device_handle(), &semaphore_info, nullptr, &semaphore));
    }
    {
      auto rendering_info = VkPipelineRenderingCreateInfo{ };
      rendering_info.sType = VK_STRUCT(PIPELINE_RENDERING_CREATE_INFO);
      rendering_info.colorAttachmentCount = 1;
      rendering_info.pColorAttachmentFormats = &m_color_format;
      rendering_info.depthAttachmentFormat = m_depth_stencil_format;
      rendering_info.stencilAttachmentFormat = m_depth_stencil_format;
      create_test_image_pipeline(rendering_info);
      create_unlit_pc_pipeline(rendering_info);
    }
    for (auto& frame : m_frames)
    {
      frame = new frame_impl{ *m_parent, m_color_format, m_depth_stencil_format, m_extent, m_pipeline_layouts,
                              m_pipelines };
    }
  }

  renderer_impl::~renderer_impl() noexcept {
    for (auto& frame : m_frames)
    {
      delete frame;
    }
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkDestroyPipeline);
    for (auto& pipeline : m_pipelines)
    {
      vkDestroyPipeline(m_parent->device_handle(), pipeline, nullptr);
    }
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkDestroyPipelineLayout);
    for (auto& layout : m_pipeline_layouts)
    {
      vkDestroyPipelineLayout(m_parent->device_handle(), layout, nullptr);
    }
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkDestroySemaphore);
    for (auto& semaphore : m_target_acquired_semaphores)
    {
      vkDestroySemaphore(m_parent->device_handle(), semaphore, nullptr);
    }
  }

  void renderer_impl::create_test_image_pipeline(const VkPipelineRenderingCreateInfo& rendering_info) {
    auto module_info = VkShaderModuleCreateInfo{ };
    module_info.sType = VK_STRUCT(SHADER_MODULE_CREATE_INFO);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreateShaderModule);
    auto pipeline_stages = std::array<VkPipelineShaderStageCreateInfo, 2>{ };
    {
      auto& pipeline_stage = pipeline_stages[0];
      pipeline_stage.sType = VK_STRUCT(PIPELINE_SHADER_STAGE_CREATE_INFO);
      pipeline_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
      pipeline_stage.pName = "main";
      module_info.pCode = shaders::test_image_vert_spv.data();
      module_info.codeSize = shaders::test_image_vert_spv.size() << 2;
      VK_SUCCEEDS(vkCreateShaderModule(m_parent->device_handle(), &module_info, nullptr, &pipeline_stage.module));
    }
    {
      auto& pipeline_stage = pipeline_stages[1];
      pipeline_stage.sType = VK_STRUCT(PIPELINE_SHADER_STAGE_CREATE_INFO);
      pipeline_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      pipeline_stage.pName = "main";
      module_info.pCode = shaders::test_image_frag_spv.data();
      module_info.codeSize = shaders::test_image_frag_spv.size() << 2;
      VK_SUCCEEDS(vkCreateShaderModule(m_parent->device_handle(), &module_info, nullptr, &pipeline_stage.module));
    }
    {
      auto graphics_pipeline_info = VkGraphicsPipelineCreateInfo{ };
      graphics_pipeline_info.sType = VK_STRUCT(GRAPHICS_PIPELINE_CREATE_INFO);
      graphics_pipeline_info.pNext = &rendering_info;
      graphics_pipeline_info.stageCount = pipeline_stages.size();
      graphics_pipeline_info.pStages = pipeline_stages.data();
      auto vertex_input = VkPipelineVertexInputStateCreateInfo{ };
      vertex_input.sType = VK_STRUCT(PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
      graphics_pipeline_info.pVertexInputState = &vertex_input;
      auto input_assembly = VkPipelineInputAssemblyStateCreateInfo{ };
      input_assembly.sType = VK_STRUCT(PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
      input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      input_assembly.primitiveRestartEnable = false;
      graphics_pipeline_info.pInputAssemblyState = &input_assembly;
      // TODO: tesselation
      auto viewport = VkPipelineViewportStateCreateInfo{ };
      viewport.sType = VK_STRUCT(PIPELINE_VIEWPORT_STATE_CREATE_INFO);
      viewport.viewportCount = 1;
      viewport.scissorCount = 1;
      graphics_pipeline_info.pViewportState = &viewport;
      auto rasterization = VkPipelineRasterizationStateCreateInfo{ };
      rasterization.sType = VK_STRUCT(PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
      rasterization.polygonMode = VK_POLYGON_MODE_FILL;
      rasterization.lineWidth = 1.0f;
      rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
      rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
      graphics_pipeline_info.pRasterizationState = &rasterization;
      auto multisample = VkPipelineMultisampleStateCreateInfo{ };
      multisample.sType = VK_STRUCT(PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
      multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
      multisample.minSampleShading = 1.0f;
      graphics_pipeline_info.pMultisampleState = &multisample;
      auto depth_stencil = VkPipelineDepthStencilStateCreateInfo{ };
      depth_stencil.sType = VK_STRUCT(PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
      depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      depth_stencil.depthTestEnable = true;
      depth_stencil.depthWriteEnable = true;
      graphics_pipeline_info.pDepthStencilState = &depth_stencil;
      auto color_blend = VkPipelineColorBlendStateCreateInfo{ };
      color_blend.sType = VK_STRUCT(PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
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
      dynamic_state.sType = VK_STRUCT(PIPELINE_DYNAMIC_STATE_CREATE_INFO);
      auto dynamic_states = std::array<VkDynamicState, 2>{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
      dynamic_state.dynamicStateCount = dynamic_states.size();
      dynamic_state.pDynamicStates = dynamic_states.data();
      graphics_pipeline_info.pDynamicState = &dynamic_state;
      auto layout_info = VkPipelineLayoutCreateInfo{ };
      layout_info.sType = VK_STRUCT(PIPELINE_LAYOUT_CREATE_INFO);
      VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreatePipelineLayout);
      VK_SUCCEEDS(vkCreatePipelineLayout(m_parent->device_handle(), &layout_info, nullptr,
                                         &m_pipeline_layouts[TEST_IMAGE_PIPELINE_INDEX]));
      graphics_pipeline_info.layout = m_pipeline_layouts[TEST_IMAGE_PIPELINE_INDEX];
      VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreateGraphicsPipelines);
      VK_SUCCEEDS(vkCreateGraphicsPipelines(m_parent->device_handle(), nullptr, 1, &graphics_pipeline_info, nullptr,
                                            &m_pipelines[TEST_IMAGE_PIPELINE_INDEX]));
    }
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkDestroyShaderModule);
    for (auto& stage : pipeline_stages)
    {
      vkDestroyShaderModule(m_parent->device_handle(), stage.module, nullptr);
    }
  }

  void renderer_impl::create_unlit_pc_pipeline(const VkPipelineRenderingCreateInfo& rendering_info) {
    auto module_info = VkShaderModuleCreateInfo{ };
    module_info.sType = VK_STRUCT(SHADER_MODULE_CREATE_INFO);
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreateShaderModule);
    auto pipeline_stages = std::array<VkPipelineShaderStageCreateInfo, 2>{ };
    {
      auto& pipeline_stage = pipeline_stages[0];
      pipeline_stage.sType = VK_STRUCT(PIPELINE_SHADER_STAGE_CREATE_INFO);
      pipeline_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
      pipeline_stage.pName = "main";
      module_info.pCode = shaders::unlit_pc_vert_spv.data();
      module_info.codeSize = shaders::unlit_pc_vert_spv.size() << 2;
      VK_SUCCEEDS(vkCreateShaderModule(m_parent->device_handle(), &module_info, nullptr, &pipeline_stage.module));
    }
    {
      auto& pipeline_stage = pipeline_stages[1];
      pipeline_stage.sType = VK_STRUCT(PIPELINE_SHADER_STAGE_CREATE_INFO);
      pipeline_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      pipeline_stage.pName = "main";
      module_info.pCode = shaders::unlit_pc_frag_spv.data();
      module_info.codeSize = shaders::unlit_pc_frag_spv.size() << 2;
      VK_SUCCEEDS(vkCreateShaderModule(m_parent->device_handle(), &module_info, nullptr, &pipeline_stage.module));
    }
    auto graphics_pipeline_info = VkGraphicsPipelineCreateInfo{ };
    graphics_pipeline_info.sType = VK_STRUCT(GRAPHICS_PIPELINE_CREATE_INFO);
    graphics_pipeline_info.pNext = &rendering_info;
    graphics_pipeline_info.stageCount = pipeline_stages.size();
    graphics_pipeline_info.pStages = pipeline_stages.data();
    auto vertex_input = VkPipelineVertexInputStateCreateInfo{ };
    vertex_input.sType = VK_STRUCT(PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
    auto binding_desc = VkVertexInputBindingDescription{ };
    binding_desc.binding = 0;
    binding_desc.stride = sizeof(vertex_pc);
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    auto vertex_attrs = std::array<VkVertexInputAttributeDescription, 2>{ };
    {
      auto& position_attr = vertex_attrs[0];
      position_attr.format = VK_FORMAT_R32G32B32A32_SFLOAT;
      position_attr.offset = offsetof(vertex_pc, position);
      position_attr.binding = 0;
      position_attr.location = 0;
    }
    {
      auto& color_attr = vertex_attrs[1];
      color_attr.format = VK_FORMAT_R32G32B32A32_SFLOAT;
      color_attr.offset = offsetof(vertex_pc, color);
      color_attr.binding = 0;
      color_attr.location = 1;
    }
    vertex_input.pVertexBindingDescriptions = &binding_desc;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexAttributeDescriptions = vertex_attrs.data();
    vertex_input.vertexAttributeDescriptionCount = vertex_attrs.size();
    graphics_pipeline_info.pVertexInputState = &vertex_input;
    auto input_assembly = VkPipelineInputAssemblyStateCreateInfo{ };
    input_assembly.sType = VK_STRUCT(PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = false;
    graphics_pipeline_info.pInputAssemblyState = &input_assembly;
    auto viewport = VkPipelineViewportStateCreateInfo{ };
    viewport.sType = VK_STRUCT(PIPELINE_VIEWPORT_STATE_CREATE_INFO);
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;
    graphics_pipeline_info.pViewportState = &viewport;
    auto rasterization = VkPipelineRasterizationStateCreateInfo{ };
    rasterization.sType = VK_STRUCT(PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.lineWidth = 1.0f;
    rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    graphics_pipeline_info.pRasterizationState = &rasterization;
    auto multisample = VkPipelineMultisampleStateCreateInfo{ };
    multisample.sType = VK_STRUCT(PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample.minSampleShading = 1.0f;
    graphics_pipeline_info.pMultisampleState = &multisample;
    auto depth_stencil = VkPipelineDepthStencilStateCreateInfo{ };
    depth_stencil.sType = VK_STRUCT(PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil.depthTestEnable = true;
    depth_stencil.depthWriteEnable = true;
    graphics_pipeline_info.pDepthStencilState = &depth_stencil;
    auto color_blend = VkPipelineColorBlendStateCreateInfo{ };
    color_blend.sType = VK_STRUCT(PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
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
    dynamic_state.sType = VK_STRUCT(PIPELINE_DYNAMIC_STATE_CREATE_INFO);
    auto dynamic_states = std::array<VkDynamicState, 2>{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    dynamic_state.dynamicStateCount = dynamic_states.size();
    dynamic_state.pDynamicStates = dynamic_states.data();
    graphics_pipeline_info.pDynamicState = &dynamic_state;
    // TODO: obviously some uniforms would be useful.
    auto layout_info = VkPipelineLayoutCreateInfo{ };
    layout_info.sType = VK_STRUCT(PIPELINE_LAYOUT_CREATE_INFO);
    auto push_constants = VkPushConstantRange{ };
    push_constants.offset = 0;
    push_constants.size = 192;
    push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constants;
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreatePipelineLayout);
    VK_SUCCEEDS(vkCreatePipelineLayout(m_parent->device_handle(), &layout_info, nullptr,
                                       &m_pipeline_layouts[UNLIT_PC_PIPELINE_INDEX]));
    graphics_pipeline_info.layout = m_pipeline_layouts[UNLIT_PC_PIPELINE_INDEX];
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkCreateGraphicsPipelines);
    VK_SUCCEEDS(vkCreateGraphicsPipelines(m_parent->device_handle(), nullptr, 1, &graphics_pipeline_info, nullptr,
                                          &m_pipelines[UNLIT_PC_PIPELINE_INDEX]));
    VK_DECLARE_PFN(m_parent->dispatch_loader(), vkDestroyShaderModule);
    for (auto& pipeline_stage : pipeline_stages)
    {
      vkDestroyShaderModule(m_parent->device_handle(), pipeline_stage.module, nullptr);
    }
  }

  frame renderer_impl::next_frame(window_impl& window) {
    const auto fr = m_frames[m_current_frame];
    fr->wait_for_availability();
    fr->begin_rendering();
    auto& info = m_infos[m_current_frame];
    info.target_acquired = m_target_acquired_semaphores[m_current_frame];
    info.window = &window;
    m_current_frame = (m_current_frame + 1) & (FRAME_COUNT - 1);
    return frame{ *fr, info };
  }

}
