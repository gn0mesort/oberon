/**
 * @file graphics.cpp
 * @brief Linux, Vulkan, X11 graphics implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/linux/graphics.hpp"

#include <cstring>

#include <unordered_set>
#include <limits>
#include <algorithm>
#include <fstream>

#include "oberon/debug.hpp"
#include "oberon/vertices.hpp"

#include "oberon/linux/system.hpp"
#include "oberon/linux/window.hpp"
#include "oberon/linux/vk.hpp"
#include "oberon/linux/vk_device.hpp"
#include "oberon/linux/buffer.hpp"

#define OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_PRECONDITIONS
/*  OBERON_PRECONDITION(m_graphics_devices.size())
*/
#define OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_POSTCONDITIONS
/*  OBERON_POSTCONDITION(m_graphics_devices.size()) \
*/
#define OBERON_LINUX_GRAPHICS_OPENED_DEVICE_PRECONDITIONS
/*  OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_PRECONDITIONS; \
  OBERON_PRECONDITION(m_vk_selected_physical_device); \
  OBERON_PRECONDITION(m_vk_device); \
  OBERON_PRECONDITION(m_vk_graphics_queue); \
  OBERON_PRECONDITION(m_vk_present_queue); \
  OBERON_PRECONDITION(m_vk_command_pools[0]); \
  OBERON_PRECONDITION(m_vk_command_pools[1])
*/
#define OBERON_LINUX_GRAPHICS_OPENED_DEVICE_POSTCONDITIONS
/*  OBERON_POSTCONDITION(m_vk_command_pools[1]); \
  OBERON_POSTCONDITION(m_vk_command_pools[0]); \
  OBERON_POSTCONDITION(m_vk_present_queue); \
  OBERON_POSTCONDITION(m_vk_graphics_queue); \
  OBERON_POSTCONDITION(m_vk_device); \
  OBERON_POSTCONDITION(m_vk_selected_physical_device); \
  OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_POSTCONDITIONS
*/
#define OBERON_LINUX_GRAPHICS_READY_TO_RENDER_PRECONDITIONS
/*  OBERON_LINUX_GRAPHICS_OPENED_DEVICE_PRECONDITIONS; \
  OBERON_PRECONDITION(m_vk_swapchain); \
  OBERON_PRECONDITION(m_vk_swapchain_images.size()); \
  OBERON_PRECONDITION(m_vk_swapchain_image_views.size() == m_vk_swapchain_images.size()); \
  OBERON_PRECONDITION(m_vk_test_image_program.program_index)
*/
#define OBERON_LINUX_GRAPHICS_READY_TO_RENDER_POSTCONDITIONS
/*  OBERON_POSTCONDITION(m_vk_test_image_program.program_index); \
  OBERON_POSTCONDITION(m_vk_swapchain_image_views.size() == m_vk_swapchain_images.size()); \
  OBERON_POSTCONDITION(m_vk_swapchain_images.size()); \
  OBERON_POSTCONDITION(m_vk_swapchain); \
  OBERON_LINUX_GRAPHICS_OPENED_DEVICE_POSTCONDITIONS
*/
#define VK_DECLARE_PFN(loader, command) OBERON_LINUX_VK_DECLARE_PFN(loader, command)
#define VK_SUCCEEDS(exp) OBERON_LINUX_VK_SUCCEEDS(exp)

namespace oberon::linux {

  graphics::graphics(system& sys, window& win) : m_parent{ &sys }, m_target{ &win } {
    auto& dl = m_parent->vk_dl();
    auto instance = m_parent->instance();
    // Load physical devices.
    auto all_physical_devices = std::vector<VkPhysicalDevice>{ };
    {
      auto sz = u32{ };
      VK_DECLARE_PFN(dl, vkEnumeratePhysicalDevices);
      VK_SUCCEEDS(vkEnumeratePhysicalDevices(instance, &sz, nullptr));
      all_physical_devices.resize(sz);
      VK_SUCCEEDS(vkEnumeratePhysicalDevices(instance, &sz, all_physical_devices.data()));
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
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceProperties2);
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceQueueFamilyProperties2);
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceSupportKHR);
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceFeatures2);
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
        {
          auto type = static_cast<graphics_device_type>(physical_device_properties2.properties.deviceType);
          auto vendor_id = physical_device_properties2.properties.vendorID;
          auto device_id = physical_device_properties2.properties.deviceID;
          auto name = std::string{ physical_device_properties2.properties.deviceName };
          auto driver_name = std::string{ physical_device_properties12.driverName };
          auto driver_info = std::string{ physical_device_properties12.driverInfo };
          // This is safe because dispatchable handles (i.e., handles defined with VK_DEFINE_HANDLE as opposed to
          // VK_DEFINE_NON_DISPATCHABLE_HANDLE) are always pointers.
          auto handle = reinterpret_cast<uptr>(physical_device);
          m_graphics_devices.emplace_back(graphics_device{ type, vendor_id, device_id, handle, name, driver_name,
                                                           driver_info });
        }
      }
    }
    // Create vk_device.
    {
      m_vk_device = create_device(preferred_device());
      initialize_graphics_pipelines();
      m_vk_device->begin_frame();
    }
    OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_POSTCONDITIONS;
  }

  graphics::~graphics() noexcept {
    delete m_vk_device;
  }

  ptr<vk_device> graphics::create_device(const graphics_device& device) {
    auto rect = m_target->current_drawable_rect();
    auto extent = VkExtent2D{ rect.extent.width, rect.extent.height };
#define PARAMS m_parent->vk_dl(), m_target->surface(), reinterpret_cast<VkPhysicalDevice>(device.handle), extent
    switch (device.vendor_id)
    {
    case 0x10de:
      return new nvidia_vk_device{ PARAMS };
    case 0x1002:
      return new amd_vk_device{ PARAMS };
    case 0x8086:
      return new intel_vk_device{ PARAMS };
    default:
      return new vk_device{ PARAMS };
    }
#undef PARAMS
  }

  std::vector<char> graphics::read_shader_binary(const std::filesystem::path& file) {
    auto f_in = std::ifstream{ file, std::ios::binary | std::ios::ate };
    auto sz = f_in.tellg();
    // SPIR-V shaders use 32-bit wide instructions?
    // At the very least VkShaderModuleCreateInfo::pCode has 32-bit wide values.
    OBERON_CHECK(!(sz & 3));
    f_in.seekg(std::ios::beg);
    auto buffer = std::vector<char>(sz);
    f_in.read(buffer.data(), buffer.size());
    return buffer;
  }

 void graphics::initialize_graphics_pipelines() {
  auto rendering_info = VkPipelineRenderingCreateInfo{ };
  rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  rendering_info.colorAttachmentCount = 1;
  auto color_format = m_vk_device->current_swapchain_format();
  auto depth_format = m_vk_device->current_depth_stencil_format();
  rendering_info.pColorAttachmentFormats = &color_format;
  rendering_info.depthAttachmentFormat = depth_format;
  initialize_test_image_pipeline(rendering_info);
  initialize_unlit_pc_pipeline(rendering_info);
 }

 void graphics::initialize_test_image_pipeline(const VkPipelineRenderingCreateInfo& rendering_info) {
    auto module_info = VkShaderModuleCreateInfo{ };
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    auto pipeline_stage = VkPipelineShaderStageCreateInfo{ };
    pipeline_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_stage.pName = "main";
    auto pipeline_stages = std::array<VkPipelineShaderStageCreateInfo, 2>{ };
    {
      auto vertex_binary = read_shader_binary(m_parent->find_file(default_file_location::immutable_data,
                                                                  "shaders/test_image.vert.spv"));
      module_info.pCode = reinterpret_cast<readonly_ptr<u32>>(vertex_binary.data());
      // Despite pCode using u32s codeSize is in bytes.
      module_info.codeSize = vertex_binary.size();
      pipeline_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
      pipeline_stage.module = m_vk_device->create_shader_module(module_info);
      pipeline_stages[0] = pipeline_stage;
    }
    auto fragment_binary = read_shader_binary(m_parent->find_file(default_file_location::immutable_data,
                                                                  "shaders/test_image.frag.spv"));
    {
      module_info.pCode = reinterpret_cast<readonly_ptr<u32>>(fragment_binary.data());
      module_info.codeSize = fragment_binary.size();
      pipeline_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      pipeline_stage.module = m_vk_device->create_shader_module(module_info);
      pipeline_stages[1] = pipeline_stage;
    }
    {
      auto graphics_pipeline_info = VkGraphicsPipelineCreateInfo{ };
      graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      graphics_pipeline_info.pNext = &rendering_info;
      graphics_pipeline_info.stageCount = pipeline_stages.size();
      graphics_pipeline_info.pStages = pipeline_stages.data();
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
      auto depth_stencil = VkPipelineDepthStencilStateCreateInfo{ };
      depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      depth_stencil.depthTestEnable = true;
      depth_stencil.depthWriteEnable = true;
      graphics_pipeline_info.pDepthStencilState = &depth_stencil;
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
      m_test_image_pipeline_layout = m_vk_device->intern_pipeline_layout(layout_info);
      graphics_pipeline_info.layout = m_test_image_pipeline_layout;
      m_test_image_pipeline = m_vk_device->intern_graphics_pipeline(graphics_pipeline_info);
      m_vk_device->destroy_shader_module(pipeline_stages[0].module);
      m_vk_device->destroy_shader_module(pipeline_stages[1].module);
    }
  }
 void graphics::initialize_unlit_pc_pipeline(const VkPipelineRenderingCreateInfo& rendering_info) {
    auto module_info = VkShaderModuleCreateInfo{ };
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    auto pipeline_stage = VkPipelineShaderStageCreateInfo{ };
    pipeline_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_stage.pName = "main";
    auto pipeline_stages = std::array<VkPipelineShaderStageCreateInfo, 2>{ };
    {
      auto vertex_binary = read_shader_binary(m_parent->find_file(default_file_location::immutable_data,
                                                                  "shaders/unlit_pc.vert.spv"));
      module_info.pCode = reinterpret_cast<readonly_ptr<u32>>(vertex_binary.data());
      // Despite pCode using u32s codeSize is in bytes.
      module_info.codeSize = vertex_binary.size();
      pipeline_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
      pipeline_stage.module = m_vk_device->create_shader_module(module_info);
      pipeline_stages[0] = pipeline_stage;
    }
    auto fragment_binary = read_shader_binary(m_parent->find_file(default_file_location::immutable_data,
                                                                  "shaders/unlit_pc.frag.spv"));
    {
      module_info.pCode = reinterpret_cast<readonly_ptr<u32>>(fragment_binary.data());
      module_info.codeSize = fragment_binary.size();
      pipeline_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      pipeline_stage.module = m_vk_device->create_shader_module(module_info);
      pipeline_stages[1] = pipeline_stage;
    }
    {
      auto graphics_pipeline_info = VkGraphicsPipelineCreateInfo{ };
      graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      graphics_pipeline_info.pNext = &rendering_info;
      graphics_pipeline_info.stageCount = pipeline_stages.size();
      graphics_pipeline_info.pStages = pipeline_stages.data();
      auto vertex_input = VkPipelineVertexInputStateCreateInfo{ };
      vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
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
      input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      input_assembly.primitiveRestartEnable = false;
      graphics_pipeline_info.pInputAssemblyState = &input_assembly;
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
      auto depth_stencil = VkPipelineDepthStencilStateCreateInfo{ };
      depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      depth_stencil.depthTestEnable = true;
      depth_stencil.depthWriteEnable = true;
      graphics_pipeline_info.pDepthStencilState = &depth_stencil;
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
      m_unlit_pc_pipeline_layout = m_vk_device->intern_pipeline_layout(layout_info);
      graphics_pipeline_info.layout = m_test_image_pipeline_layout;
      m_unlit_pc_pipeline = m_vk_device->intern_graphics_pipeline(graphics_pipeline_info);
      m_vk_device->destroy_shader_module(pipeline_stages[0].module);
      m_vk_device->destroy_shader_module(pipeline_stages[1].module);
    }
  }

  u32 graphics::current_buffer_count() const {
    OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_PRECONDITIONS;
    return m_vk_device->current_swapchain_size();
  }

  void graphics::request_buffer_count(const u32 buffers) {
    OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_PRECONDITIONS;
    m_vk_device->request_swapchain_images(buffers);
  }

  const std::unordered_set<presentation_mode>& graphics::available_presentation_modes() const {
    OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_PRECONDITIONS;
    throw not_implemented_error{ };
  }

  presentation_mode graphics::current_presentation_mode() const {
    OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_PRECONDITIONS;
    return static_cast<presentation_mode>(static_cast<usize>(m_vk_device->current_present_mode()) + 1);
  }

  void graphics::request_presentation_mode(const presentation_mode mode) {
    OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_PRECONDITIONS;
    m_vk_device->request_present_mode(static_cast<VkPresentModeKHR>(static_cast<usize>(mode) - 1));
  }

  const std::vector<graphics_device>& graphics::available_devices() const {
    OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_PRECONDITIONS;
    return m_graphics_devices;
  }

  const graphics_device& graphics::preferred_device() const {
    OBERON_LINUX_GRAPHICS_CLOSED_DEVICE_PRECONDITIONS;
    const auto sz = m_graphics_devices.size();
    auto result_index = sz;
    for (auto i = usize{ 0 }; i < sz && result_index; ++i)
    {
      const auto& device = m_graphics_devices[i];
      const auto is_discrete = device.type == graphics_device_type::discrete;
      // If is_discrete then result_index = i
      // Else result_index = m_graphics_devices.size()
      result_index = (i & -is_discrete) + (sz & -!is_discrete);
    }
    // If result_index != sz then return m_graphics_devices[result_index]
    // Else return m_graphics_devices[0]
    return m_graphics_devices[result_index & -(result_index != sz)];
  }

  void graphics::change_device(const graphics_device& device) {
    delete m_vk_device;
    m_vk_device = create_device(device);
    initialize_graphics_pipelines();
  }

  void graphics::draw_test_image() {
    m_vk_device->draw(m_test_image_pipeline, 3);
  }

  void graphics::submit_and_present_frame() {
    m_vk_device->end_frame();
    m_vk_device->begin_frame();
  }

  vk_device& graphics::device() {
    return *m_vk_device;
  }

  void graphics::draw_buffer_unlit_pc(oberon::buffer& buf) {
    auto& linbuf = static_cast<buffer&>(buf);
    m_vk_device->draw_buffer(m_unlit_pc_pipeline, linbuf.resident(), linbuf.size() / sizeof(vertex_pc));
  }

  oberon::buffer& graphics::allocate_buffer(const buffer_type type, const usize sz) {
    return *(new buffer{ type, *m_vk_device, sz });
  }

  void graphics::free_buffer(oberon::buffer& buf) {
    delete &buf;
  }

  void graphics::flush_device_queues() const {
    m_vk_device->wait_device_idle();
  }

}
