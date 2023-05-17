#include "oberon/internal/linux/x11/render_window_impl.hpp"

#include <cstring>

#include <algorithm>

#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <nng/protocol/pubsub0/sub.h>

#include "oberon/debug.hpp"
#include "oberon/errors.hpp"
#include "oberon/utility.hpp"
#include "oberon/graphics_device.hpp"
#include "oberon/vertices.hpp"
#include "oberon/camera.hpp"
#include "oberon/mesh.hpp"

#include "oberon/internal/base/mesh_impl.hpp"

#include "oberon/internal/linux/x11/xcb.hpp"
#include "oberon/internal/linux/x11/atoms.hpp"
#include "oberon/internal/linux/x11/keys.hpp"
#include "oberon/internal/linux/x11/wsi_context.hpp"
#include "oberon/internal/linux/x11/wsi_worker.hpp"
#include "oberon/internal/linux/x11/graphics_device_impl.hpp"

#include "test_image_vert_spv.hpp"
#include "test_image_frag_spv.hpp"
#include "unlit_pc_vert_spv.hpp"
#include "unlit_pc_frag_spv.hpp"

#define XCB_SEND_REQUEST_SYNC(reply, request, connection, ...) \
  OBERON_INTERNAL_LINUX_X11_XCB_SEND_REQUEST_SYNC(reply, request, connection __VA_OPT__(, __VA_ARGS__))

#define XCB_SEND_REQUEST(request, connection, ...) \
  OBERON_INTERNAL_LINUX_X11_XCB_SEND_REQUEST(request, connection __VA_OPT__(, __VA_ARGS__))

#define XCB_AWAIT_REPLY(request, connection, cookie, error) \
  OBERON_INTERNAL_LINUX_X11_XCB_AWAIT_REPLY(request, connection, cookie, error)

#define XCB_HANDLE_ERROR(reply, error, msg) \
  OBERON_INTERNAL_LINUX_X11_XCB_HANDLE_ERROR(reply, error, msg)

#define VK_STRUCT(name) \
  OBERON_INTERNAL_BASE_VK_STRUCT(name)

#define VK_DECLARE_PFN(dl, name) \
  OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, name)

#define VK_SUCCEEDS(exp) \
  OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp)

namespace oberon::internal::linux::x11 {

  render_window_impl::render_window_impl(graphics_device& device, const std::string& title, const rect_2d& bounds) :
  m_parent_device{ &device } {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    const auto screen = parent.wsi().default_screen();
    // Create the X11 window.
    m_window = xcb_generate_id(connection);
    constexpr const auto EVENT_MASK = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                                      XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW |
                                      XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE;
    const auto values = std::array<u32, 3>{ screen->black_pixel, false, EVENT_MASK };
    xcb_create_window(connection, XCB_COPY_FROM_PARENT, m_window, screen->root, bounds.offset.x, bounds.offset.y,
                      bounds.extent.width, bounds.extent.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
                      values.data());
    // Configure XInput 2 events.
    {
      auto masks = std::array<xcb_input_xi_event_mask_list_t, 2>{ };
      masks[0].head.deviceid = parent.wsi().keyboard();
      masks[0].head.mask_len = sizeof(xcb_input_xi_event_mask_t) >> 2;
      masks[0].mask = static_cast<xcb_input_xi_event_mask_t>(XCB_INPUT_XI_EVENT_MASK_KEY_PRESS |
                                                             XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE);
      masks[1].head.deviceid = parent.wsi().pointer();
      masks[1].head.mask_len = sizeof(xcb_input_xi_event_mask_t) >> 2;
      masks[1].mask = static_cast<xcb_input_xi_event_mask_t>(XCB_INPUT_XI_EVENT_MASK_MOTION |
                                                             XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS |
                                                             XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE);
      xcb_input_xi_select_events(connection, m_window, masks.size(), &masks[0].head);
    }
    // Connect to the event worker thread via NNG.
    OBERON_CHECK_ERROR_MSG(!nng_sub0_open(&m_socket), 1, "Failed to open an NNG subscriber socket.");
    {
      auto res = nng_socket_set(m_socket, NNG_OPT_SUB_SUBSCRIBE, &m_window, sizeof(xcb_window_t));
      auto leader = parent.wsi().leader();
      res += nng_socket_set(m_socket, NNG_OPT_SUB_SUBSCRIBE, &leader, sizeof(xcb_window_t));
      OBERON_CHECK_ERROR_MSG(!res, 1, "Failed to subscribe to the desired window events.");
    }
    OBERON_CHECK_ERROR_MSG(!nng_dial(m_socket, WSI_WORKER_ENDPOINT, &m_dialer, 0), 1, "Failed to dial the event "
                           "worker thread.");
    // Configure the window.
    {
      const auto instance = parent.wsi().instance_name();
      const auto application = parent.wsi().application_name();
      // Set WM_CLASS
      // WM_CLASS is an odd value in that both strings are explicitly null terminated (i.e., cstrings). Other string
      // atoms don't necessarily care about the null terminator.
      // Additionally, the first value (i.e., the instance name) must be acquired in accordance with ICCCM. This means
      // that the -name argument is checked first, then RESOURCE_NAME, and finally basename(argv[0]) is used.
      // see https://www.x.org/releases/current/doc/xorg-docs/icccm/icccm.html#WM_CLASS_Property
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(WM_CLASS_ATOM),
                          XCB_ATOM_STRING, 8, instance.size() + 1, instance.c_str());
      xcb_change_property(connection, XCB_PROP_MODE_APPEND, m_window, parent.wsi().atom_by_name(WM_CLASS_ATOM),
                          XCB_ATOM_STRING, 8, application.size() + 1, application.c_str());
    }
    {
      const auto pid = getpid();
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(NET_WM_PID_ATOM),
                          XCB_ATOM_CARDINAL, 32, 1, &pid);
      auto buffer = utsname{ };
      OBERON_CHECK_ERROR_MSG(!uname(&buffer), 1, "Failed to retrieve machine hostname.");
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window,
                          parent.wsi().atom_by_name(WM_CLIENT_MACHINE_ATOM), XCB_ATOM_STRING, 8,
                          std::strlen(buffer.nodename), &buffer.nodename[0]);
    }
    {
      const auto protocols_atom = parent.wsi().atom_by_name(WM_PROTOCOLS_ATOM);
      const auto atoms = std::array<xcb_atom_t, 2>{ parent.wsi().atom_by_name(WM_DELETE_WINDOW_ATOM),
                                                    parent.wsi().atom_by_name(NET_WM_PING_ATOM) };
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, protocols_atom, XCB_ATOM_ATOM, 32, 2,
                          atoms.data());
    }
    change_title(title);
    // Set WM_NORMAL_HINTS
    {
      auto hints = xcb_size_hints_t{ };
      // Set max/min size and inform the WM that size/position is a user choice.
      hints.flags = size_hint_flag_bits::program_max_size_bit | size_hint_flag_bits::program_min_size_bit |
                    size_hint_flag_bits::user_size_bit | size_hint_flag_bits::user_position_bit;
      hints.max_width = bounds.extent.width;
      hints.min_width = bounds.extent.width;
      hints.max_height = bounds.extent.height;
      hints.min_height = bounds.extent.height;
      change_size_hints(hints);
    }
    // Set WM_CLIENT_LEADER
    {
      const auto leader = parent.wsi().leader();
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window,
                          parent.wsi().atom_by_name(WM_CLIENT_LEADER_ATOM), XCB_ATOM_WINDOW, 32, 1, &leader);
    }
    // Set WM_HINTS
    {
      auto hints = xcb_hints_t{ };
      hints.flags = hint_flag_bits::window_group_hint_bit | hint_flag_bits::state_hint_bit;
      hints.initial_state = NORMAL_STATE;
      hints.window_group = parent.wsi().leader();
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(WM_HINTS_ATOM),
                          XCB_ATOM_WM_HINTS, 32, sizeof(xcb_hints_t) >> 2, &hints);
    }
    // Set _NET_WM_BYPASS_COMPOSITOR
    {
      const auto mode = u32{ NO_PREFERENCE_COMPOSITOR };
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window,
                          parent.wsi().atom_by_name(NET_WM_BYPASS_COMPOSITOR_ATOM), XCB_ATOM_CARDINAL, 32, 1, &mode);
    }
    // Get pointer map.
    // This doesn't seem to be allowed to change in current servers for "security" reasons.
    // I'm only using this to discover the actual number of pointer buttons.
    // My system has 20 but I know mice with more buttons exist and X11 just says, "pointer buttons are always
    // numbered starting at 1."
    // see: https://www.x.org/releases/current/doc/xproto/x11protocol.html#Pointers
    {
      auto reply = ptr<xcb_get_pointer_mapping_reply_t>{ };
      XCB_SEND_REQUEST_SYNC(reply, xcb_get_pointer_mapping, connection);
      m_mouse_button_states.resize(xcb_get_pointer_mapping_map_length(reply));
      std::free(reply);
    }
    initialize_keyboard();
    const auto& dl = parent.dispatch_loader();
    const auto physical = parent.physical_device().handle();
    // Create Vulkan window surface.
    {
      auto surface_info = VkXcbSurfaceCreateInfoKHR{ };
      surface_info.sType = VK_STRUCT(XCB_SURFACE_CREATE_INFO_KHR);
      surface_info.connection = connection;
      surface_info.window = m_window;
      VK_DECLARE_PFN(dl, vkCreateXcbSurfaceKHR);
      VK_SUCCEEDS(vkCreateXcbSurfaceKHR(parent.instance_handle(), &surface_info, nullptr, &m_surface));
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceSupportKHR);
      auto supported = VkBool32{ };
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceSupportKHR(physical, parent.queue_family(), m_surface, &supported));
      OBERON_CHECK_ERROR_MSG(supported, 1, "The selected device queue family (%u) does not support the "
                             "render_window.", parent.queue_family());
    }
    // Select Vulkan surface image format.
    {
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceFormatsKHR);
      auto sz = u32{ };
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, m_surface, &sz, nullptr));
      auto surface_formats = std::vector<VkSurfaceFormatKHR>(sz);
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, m_surface, &sz, surface_formats.data()));
      auto found = false;
      for (auto i = u32{ 0 }; i < surface_formats.size() && !found; ++i)
      {
        // According to gpuinfo.org these are the two most commonly supported SRGB surface formats across all
        // platforms. About 63% of reports support R8G8B8A8_SRGB and about 45% support B8G8R8A8_SRGB.
        const auto is_desired = surface_formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                                surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB;
        const auto is_srgb = surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        if (is_desired && is_srgb)
        {
          m_swapchain_surface_format = surface_formats[i];
          found = true;
        }
      }
      // If neither of the desired formats are available then the best option is to pick the first format.
      // Oh well!
      if (!found)
      {
        m_swapchain_surface_format = surface_formats.front();
      }
    }
    // Select Vulkan depth-stencil format.
    {
      // The preference values here are arbitrary. They only define the ordering of the preferences.
      // On Linux systems, 99% of devices support D32_SFLOAT_S8_UINT. Approximately 50% of devices support
      // D24_UNORM_S8_UINT and D16_UNORM_S8_UINT.
      m_depth_stencil_format = parent.select_image_format({ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT,
                                                            VK_FORMAT_D16_UNORM_S8_UINT },VK_IMAGE_TILING_OPTIMAL,
                                                          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
      OBERON_CHECK_ERROR_MSG(m_depth_stencil_format != VK_FORMAT_UNDEFINED, 1, "Failed to find an acceptable "
                             "depth-stencil image format.");
    }
    // Find available present modes.
    {
      auto sz  = u32{ };
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfacePresentModesKHR);
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfacePresentModesKHR(parent.physical_device().handle(), m_surface, &sz,
                                                            nullptr));
      auto modes = std::vector<VkPresentModeKHR>(sz);
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfacePresentModesKHR(parent.physical_device().handle(), m_surface, &sz,
                                                            modes.data()));
      for (const auto& mode : modes)
      {
        m_presentation_modes.insert(static_cast<presentation_mode>(mode));
      }
    }
    // Initialize Vulkan swapchain
    initialize_swapchain(VK_NULL_HANDLE);
    // Initialize depth-stencil images.
    initialize_depth_stencil();
    // Initialize Vulkan synchronization objects.
    const auto handle = parent.device_handle();
    {
      auto fence_info = VkFenceCreateInfo{ };
      fence_info.sType = VK_STRUCT(FENCE_CREATE_INFO);
      fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      VK_DECLARE_PFN(dl, vkCreateFence);
      for (auto& fence : m_frame_fences)
      {
        VK_SUCCEEDS(vkCreateFence(handle, &fence_info, nullptr, &fence));
      }
      auto semaphore_info = VkSemaphoreCreateInfo{ };
      semaphore_info.sType = VK_STRUCT(SEMAPHORE_CREATE_INFO);
      VK_DECLARE_PFN(dl, vkCreateSemaphore);
      for (auto& semaphore : m_frame_semaphores)
      {
        VK_SUCCEEDS(vkCreateSemaphore(handle, &semaphore_info, nullptr, &semaphore));
      }
    }
    // Initialize Vulkan command pools and command buffers
    {
      auto command_pool_info = VkCommandPoolCreateInfo{ };
      command_pool_info.sType = VK_STRUCT(COMMAND_POOL_CREATE_INFO);
      command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
      command_pool_info.queueFamilyIndex = parent.queue_family();
      auto command_buffer_info = VkCommandBufferAllocateInfo{ };
      command_buffer_info.sType = VK_STRUCT(COMMAND_BUFFER_ALLOCATE_INFO);
      command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      command_buffer_info.commandBufferCount = COMMAND_BUFFERS_PER_FRAME;
      VK_DECLARE_PFN(dl, vkCreateCommandPool);
      VK_DECLARE_PFN(dl, vkAllocateCommandBuffers);
      for (auto i = u32{ 0 }; i < FRAME_COUNT; ++i)
      {
        VK_SUCCEEDS(vkCreateCommandPool(handle, &command_pool_info, nullptr, &m_frame_command_pools[i]));
        command_buffer_info.commandPool = m_frame_command_pools[i];
        const auto command_buffer_addr = &m_frame_command_buffers[i * COMMAND_BUFFERS_PER_FRAME];
        VK_SUCCEEDS(vkAllocateCommandBuffers(handle, &command_buffer_info, command_buffer_addr));
      }
    }
    // Initialize Vulkan pipelines
    {
      auto rendering_info = VkPipelineRenderingCreateInfo{ };
      rendering_info.sType = VK_STRUCT(PIPELINE_RENDERING_CREATE_INFO);
      rendering_info.colorAttachmentCount = 1;
      rendering_info.pColorAttachmentFormats = &m_swapchain_surface_format.format;
      rendering_info.depthAttachmentFormat = m_depth_stencil_format;
      rendering_info.stencilAttachmentFormat = m_depth_stencil_format;
      VK_DECLARE_PFN(dl, vkCreateDescriptorSetLayout);
      {
        auto camera_layout = VkDescriptorSetLayoutCreateInfo{ };
        camera_layout.sType = VK_STRUCT(DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
        camera_layout.bindingCount = 1;
        auto camera_binding = VkDescriptorSetLayoutBinding{ };
        camera_binding.binding = 0;
        camera_binding.stageFlags = VK_SHADER_STAGE_ALL;
        camera_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        camera_binding.descriptorCount = 1;
        camera_layout.pBindings = &camera_binding;
        VK_SUCCEEDS(vkCreateDescriptorSetLayout(handle, &camera_layout, nullptr, &m_camera_descriptor_layout));
      }
      {
        auto mesh_layout = VkDescriptorSetLayoutCreateInfo{ };
        mesh_layout.sType = VK_STRUCT(DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
        mesh_layout.bindingCount = 1;
        auto mesh_binding = VkDescriptorSetLayoutBinding{ };
        mesh_binding.binding = 0;
        mesh_binding.stageFlags = VK_SHADER_STAGE_ALL;
        mesh_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        mesh_binding.descriptorCount = 1;
        mesh_layout.pBindings = &mesh_binding;
        VK_SUCCEEDS(vkCreateDescriptorSetLayout(handle, &mesh_layout, nullptr, &m_mesh_descriptor_layout));
      }
      auto descriptor_layouts = std::array<VkDescriptorSetLayout, 2>{ m_camera_descriptor_layout,
                                                                      m_mesh_descriptor_layout };
      // Test Image
      {
        auto module_info = VkShaderModuleCreateInfo{ };
        module_info.sType = VK_STRUCT(SHADER_MODULE_CREATE_INFO);
        VK_DECLARE_PFN(dl, vkCreateShaderModule);
        auto pipeline_stages = std::array<VkPipelineShaderStageCreateInfo, 2>{ };
        {
          auto& pipeline_stage = pipeline_stages[0];
          pipeline_stage.sType = VK_STRUCT(PIPELINE_SHADER_STAGE_CREATE_INFO);
          pipeline_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
          pipeline_stage.pName = "main";
          module_info.pCode = shaders::test_image_vert_spv.data();
          module_info.codeSize = shaders::test_image_vert_spv.size() << 2;
          VK_SUCCEEDS(vkCreateShaderModule(handle, &module_info, nullptr, &pipeline_stage.module));
        }
        {
          auto& pipeline_stage = pipeline_stages[1];
          pipeline_stage.sType = VK_STRUCT(PIPELINE_SHADER_STAGE_CREATE_INFO);
          pipeline_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
          pipeline_stage.pName = "main";
          module_info.pCode = shaders::test_image_frag_spv.data();
          module_info.codeSize = shaders::test_image_frag_spv.size() << 2;
          VK_SUCCEEDS(vkCreateShaderModule(handle, &module_info, nullptr, &pipeline_stage.module));
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
          VK_DECLARE_PFN(dl, vkCreatePipelineLayout);
          VK_SUCCEEDS(vkCreatePipelineLayout(handle, &layout_info, nullptr, &m_test_image_pipeline_layout));
          graphics_pipeline_info.layout = m_test_image_pipeline_layout;
          VK_DECLARE_PFN(dl, vkCreateGraphicsPipelines);
          VK_SUCCEEDS(vkCreateGraphicsPipelines(handle, nullptr, 1, &graphics_pipeline_info, nullptr,
                                                &m_test_image_pipeline));
        }
        VK_DECLARE_PFN(dl, vkDestroyShaderModule);
        for (auto& stage : pipeline_stages)
        {
          vkDestroyShaderModule(handle, stage.module, nullptr);
        }
      }
      // Unlit position, color
      {
        auto module_info = VkShaderModuleCreateInfo{ };
        module_info.sType = VK_STRUCT(SHADER_MODULE_CREATE_INFO);
        VK_DECLARE_PFN(dl, vkCreateShaderModule);
        auto pipeline_stages = std::array<VkPipelineShaderStageCreateInfo, 2>{ };
        {
          auto& pipeline_stage = pipeline_stages[0];
          pipeline_stage.sType = VK_STRUCT(PIPELINE_SHADER_STAGE_CREATE_INFO);
          pipeline_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
          pipeline_stage.pName = "main";
          module_info.pCode = shaders::unlit_pc_vert_spv.data();
          module_info.codeSize = shaders::unlit_pc_vert_spv.size() << 2;
          VK_SUCCEEDS(vkCreateShaderModule(handle, &module_info, nullptr, &pipeline_stage.module));
        }
        {
          auto& pipeline_stage = pipeline_stages[1];
          pipeline_stage.sType = VK_STRUCT(PIPELINE_SHADER_STAGE_CREATE_INFO);
          pipeline_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
          pipeline_stage.pName = "main";
          module_info.pCode = shaders::unlit_pc_frag_spv.data();
          module_info.codeSize = shaders::unlit_pc_frag_spv.size() << 2;
          VK_SUCCEEDS(vkCreateShaderModule(handle, &module_info, nullptr, &pipeline_stage.module));
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
        layout_info.pSetLayouts = descriptor_layouts.data();
        layout_info.setLayoutCount = descriptor_layouts.size();
        VK_DECLARE_PFN(dl, vkCreatePipelineLayout);
        VK_SUCCEEDS(vkCreatePipelineLayout(handle, &layout_info, nullptr, &m_unlit_pc_pipeline_layout));
        graphics_pipeline_info.layout = m_unlit_pc_pipeline_layout;
        VK_DECLARE_PFN(dl, vkCreateGraphicsPipelines);
        VK_SUCCEEDS(vkCreateGraphicsPipelines(handle, nullptr, 1, &graphics_pipeline_info, nullptr,
                                              &m_unlit_pc_pipeline));
        VK_DECLARE_PFN(dl, vkDestroyShaderModule);
        for (auto& pipeline_stage : pipeline_stages)
        {
          vkDestroyShaderModule(handle, pipeline_stage.module, nullptr);
        }
      }
      // Create descriptor sets
      {
        auto descriptor_pool_info = VkDescriptorPoolCreateInfo{ };
        descriptor_pool_info.sType = VK_STRUCT(DESCRIPTOR_POOL_CREATE_INFO);
        descriptor_pool_info.maxSets = m_frame_descriptor_sets.size();
        auto pool_size = VkDescriptorPoolSize{ };
        pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_size.descriptorCount = m_frame_descriptor_sets.size();
        descriptor_pool_info.poolSizeCount = 1;
        descriptor_pool_info.pPoolSizes = &pool_size;
        VK_DECLARE_PFN(dl, vkCreateDescriptorPool);
        VK_SUCCEEDS(vkCreateDescriptorPool(handle, &descriptor_pool_info, nullptr, &m_frame_descriptor_pool));
        auto descriptor_set_info = VkDescriptorSetAllocateInfo{ };
        descriptor_set_info.sType = VK_STRUCT(DESCRIPTOR_SET_ALLOCATE_INFO);
        descriptor_set_info.descriptorSetCount = descriptor_layouts.size();
        descriptor_set_info.pSetLayouts = descriptor_layouts.data();
        descriptor_set_info.descriptorPool = m_frame_descriptor_pool;
        VK_DECLARE_PFN(dl, vkAllocateDescriptorSets);
        for (auto i = usize{ 0 }; i < DESCRIPTOR_SETS_PER_FRAME; ++i)
        {
          const auto index = (i * DESCRIPTOR_SETS_PER_FRAME) + CAMERA_DESCRIPTOR_SET_INDEX;
          VK_SUCCEEDS(vkAllocateDescriptorSets(handle, &descriptor_set_info, &m_frame_descriptor_sets[index]));
        }
      }
    }
    // Prepare rendering info
    for (auto i = usize{ 0 }; i < FRAME_COUNT; ++i)
    {
      const auto base = (i * ATTACHMENTS_PER_FRAME);
      auto& color_attachment_info = m_frame_attachments[base + COLOR_ATTACHMENT_INDEX];
      color_attachment_info.sType = VK_STRUCT(RENDERING_ATTACHMENT_INFO);
      color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      // This is a neutral gray color. It's important that the clear color is not black because many render errors will
      // output black pixels.
      // This color is interpretted as being in linear colorspace and not sRGB.
      color_attachment_info.clearValue.color = { { to_linear_color(0.2f), to_linear_color(0.2f),
                                                   to_linear_color(0.2f) } };
      //color_attachment_info.imageView = m_swapchain_image_views[i];
      auto& depth_attachment_info = m_frame_attachments[base + DEPTH_ATTACHMENT_INDEX];
      depth_attachment_info.sType = VK_STRUCT(RENDERING_ATTACHMENT_INFO);
      depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      depth_attachment_info.clearValue.depthStencil.depth = 1.0f;
      depth_attachment_info.imageView = m_frame_depth_stencil_image_views[i];
      auto& stencil_attachment_info = m_frame_attachments[base + STENCIL_ATTACHMENT_INDEX];
      stencil_attachment_info.sType = VK_STRUCT(RENDERING_ATTACHMENT_INFO);
      stencil_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      stencil_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      stencil_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      stencil_attachment_info.clearValue.depthStencil.stencil = 0;
      stencil_attachment_info.imageView = m_frame_depth_stencil_image_views[i];
      auto& rendering_info = m_frame_rendering_info[i];
      rendering_info.sType = VK_STRUCT(RENDERING_INFO);
      rendering_info.renderArea.offset = { 0, 0 };
      rendering_info.renderArea.extent = m_swapchain_extent;
      rendering_info.layerCount = 1;
      rendering_info.colorAttachmentCount = 1;
      rendering_info.pColorAttachments = &color_attachment_info;
      rendering_info.pDepthAttachment = &depth_attachment_info;
      rendering_info.pStencilAttachment = &stencil_attachment_info;
    }
    begin_frame();
  }

  render_window_impl::~render_window_impl() noexcept {
    hide();
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto& dl = parent.dispatch_loader();
    const auto device = parent.device_handle();
    VK_DECLARE_PFN(dl, vkWaitForFences);
    // Although this can fail, I'm ignoring failure since destruction must occur either way.
    vkWaitForFences(device, m_frame_fences.size(), m_frame_fences.data(), true, VK_FOREVER);
    if (m_active_camera)
    {
      m_active_camera->detach_window(m_active_camera_cookie);
    }
    VK_DECLARE_PFN(dl, vkDestroyPipeline);
    VK_DECLARE_PFN(dl, vkDestroyDescriptorPool);
    vkDestroyDescriptorPool(device, m_frame_descriptor_pool, nullptr);
    vkDestroyPipeline(device, m_test_image_pipeline, nullptr);
    vkDestroyPipeline(device, m_unlit_pc_pipeline, nullptr);
    VK_DECLARE_PFN(dl, vkDestroyPipelineLayout);
    vkDestroyPipelineLayout(device, m_test_image_pipeline_layout, nullptr);
    vkDestroyPipelineLayout(device, m_unlit_pc_pipeline_layout, nullptr);
    VK_DECLARE_PFN(dl, vkDestroyDescriptorSetLayout);
    vkDestroyDescriptorSetLayout(device, m_mesh_descriptor_layout, nullptr);
    vkDestroyDescriptorSetLayout(device, m_camera_descriptor_layout, nullptr);
    VK_DECLARE_PFN(dl, vkFreeCommandBuffers);
    VK_DECLARE_PFN(dl, vkDestroyCommandPool);
    for (auto i = u32{ 0 }; i < FRAME_COUNT; ++i)
    {
      const auto command_buffer_addr = &m_frame_command_buffers[i * COMMAND_BUFFERS_PER_FRAME];
      vkFreeCommandBuffers(device, m_frame_command_pools[i], COMMAND_BUFFERS_PER_FRAME, command_buffer_addr);
      vkDestroyCommandPool(device, m_frame_command_pools[i], nullptr);
    }
    VK_DECLARE_PFN(dl, vkDestroySemaphore);
    for (auto& semaphore : m_frame_semaphores)
    {
      vkDestroySemaphore(device, semaphore, nullptr);
    }
    VK_DECLARE_PFN(dl, vkDestroyFence);
    for (auto& fence : m_frame_fences)
    {
      vkDestroyFence(device, fence, nullptr);
    }
    deinitialize_depth_stencil();
    VK_DECLARE_PFN(dl, vkDestroyImageView);
    for (auto& image_view : m_swapchain_image_views)
    {
      vkDestroyImageView(device, image_view, nullptr);
    }
    VK_DECLARE_PFN(dl, vkDestroySwapchainKHR);
    vkDestroySwapchainKHR(device, m_swapchain, nullptr);
    VK_DECLARE_PFN(dl, vkDestroySurfaceKHR);
    vkDestroySurfaceKHR(parent.instance_handle(), m_surface, nullptr);
    deinitialize_keyboard();
    xcb_destroy_window(parent.wsi().connection(), m_window);
    nng_dialer_close(m_dialer);
    nng_close(m_socket);
  }

  void render_window_impl::change_size_hints(const xcb_size_hints_t& hints) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(WM_NORMAL_HINTS_ATOM),
                        XCB_ATOM_WM_SIZE_HINTS, 32, sizeof(xcb_size_hints_t) >> 2, &hints);
  }

  void render_window_impl::initialize_depth_stencil() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto& dl = parent.dispatch_loader();
    const auto device = parent.device_handle();
    auto allocation_info = VmaAllocationCreateInfo{ };
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocation_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    auto image_info = VkImageCreateInfo{ };
    image_info.sType = VK_STRUCT(IMAGE_CREATE_INFO);
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.extent.width = m_swapchain_extent.width;
    image_info.extent.height = m_swapchain_extent.height;
    image_info.extent.depth  = 1;
    image_info.format = m_depth_stencil_format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    for (auto& image : m_frame_depth_stencil_images)
    {
      image = parent.create_image(image_info, allocation_info);
    }
    VK_DECLARE_PFN(dl, vkCreateImageView);
    auto image_view_info = VkImageViewCreateInfo{ };
    image_view_info.sType = VK_STRUCT(IMAGE_VIEW_CREATE_INFO);
    image_view_info.format = m_depth_stencil_format;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.layerCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    auto cur = m_frame_depth_stencil_images.begin();
    for (auto& image_view : m_frame_depth_stencil_image_views)
    {
      image_view_info.image = (*cur)->image;
      VK_SUCCEEDS(vkCreateImageView(device, &image_view_info, nullptr, &image_view));
       ++cur;
    }
    for (auto i = usize{ 0 }; i < FRAME_COUNT; ++i)
    {
      const auto base = (i * ATTACHMENTS_PER_FRAME);
      m_frame_attachments[base + DEPTH_ATTACHMENT_INDEX].imageView = m_frame_depth_stencil_image_views[i];
      m_frame_attachments[base + STENCIL_ATTACHMENT_INDEX].imageView = m_frame_depth_stencil_image_views[i];
    }
  }

  void render_window_impl::deinitialize_depth_stencil() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto& dl = parent.dispatch_loader();
    const auto device = parent.device_handle();
    VK_DECLARE_PFN(dl, vkDestroyImageView);
    for (auto& image_view : m_frame_depth_stencil_image_views)
    {
      vkDestroyImageView(device, image_view, nullptr);
    }
    for (auto& image : m_frame_depth_stencil_images)
    {
      parent.destroy_image(image);
    }
  }

  void render_window_impl::initialize_swapchain(const VkSwapchainKHR old) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto& dl = parent.dispatch_loader();
    const auto device = parent.device_handle();
    VK_DECLARE_PFN(dl, vkDestroyImageView);
    for (auto& image_view : m_swapchain_image_views)
    {
      vkDestroyImageView(device, image_view, nullptr);
    }
    m_swapchain_image_views.clear();
    m_swapchain_images.clear();
    auto swapchain_info = VkSwapchainCreateInfoKHR{ };
    swapchain_info.sType = VK_STRUCT(SWAPCHAIN_CREATE_INFO_KHR);
    swapchain_info.clipped = true;
    swapchain_info.surface = m_surface;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    auto capabilities = VkSurfaceCapabilitiesKHR{ };
    VK_DECLARE_PFN(parent.dispatch_loader(), vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(parent.physical_device().handle(), m_surface,
                                                          &capabilities));
    // Per the Vulkan specification this must be true.
    // See section 34.2.4 "XCB Platform"
    OBERON_ASSERT(capabilities.currentExtent.width == capabilities.minImageExtent.width);
    OBERON_ASSERT(capabilities.currentExtent.width == capabilities.maxImageExtent.width);
    OBERON_ASSERT(capabilities.currentExtent.height == capabilities.minImageExtent.height);
    OBERON_ASSERT(capabilities.currentExtent.height == capabilities.maxImageExtent.height);
    // TODO: Handle the odd case of a (0, 0) extent which is technically possible according to the specification.
    swapchain_info.imageExtent = m_swapchain_extent = capabilities.currentExtent;
    swapchain_info.imageFormat = m_swapchain_surface_format.format;
    swapchain_info.presentMode = m_swapchain_present_mode;
    swapchain_info.oldSwapchain = old;
    // This is useful for cellphones or other devices where the screen can be rotated (allegedly).
    swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    // The max image count *can* be 0 to indicate any number of images (greater than the minimum) is supported.
    const auto max_images = capabilities.maxImageCount ? capabilities.maxImageCount : std::numeric_limits<u32>::max();
    // TODO: Enable buffer count selection even though the API will *probably* ignore it.
    swapchain_info.minImageCount = std::clamp(u32{ 3 }, capabilities.minImageCount, max_images);
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.imageColorSpace = m_swapchain_surface_format.colorSpace;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    auto queue_family = parent.queue_family();
    swapchain_info.pQueueFamilyIndices = &queue_family;
    swapchain_info.queueFamilyIndexCount = 1;
    VK_DECLARE_PFN(dl, vkCreateSwapchainKHR);
    VK_SUCCEEDS(vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &m_swapchain));
    VK_DECLARE_PFN(dl, vkGetSwapchainImagesKHR);
    auto sz = u32{ };
    VK_SUCCEEDS(vkGetSwapchainImagesKHR(device, m_swapchain, &sz, nullptr));
    m_swapchain_images.resize(sz);
    VK_SUCCEEDS(vkGetSwapchainImagesKHR(device, m_swapchain, &sz, m_swapchain_images.data()));
    m_swapchain_image_views.resize(sz);
    auto image_view_info = VkImageViewCreateInfo{ };
    image_view_info.sType = VK_STRUCT(IMAGE_VIEW_CREATE_INFO);
    image_view_info.format = m_swapchain_surface_format.format;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.layerCount = 1;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    VK_DECLARE_PFN(dl, vkCreateImageView);
    auto cur = m_swapchain_image_views.begin();
    for (const auto& image : m_swapchain_images)
    {
      image_view_info.image = image;
      auto& image_view = *cur;
      VK_SUCCEEDS(vkCreateImageView(device, &image_view_info, nullptr, &image_view));
      ++cur;
    }
    for (auto& rendering_info : m_frame_rendering_info)
    {
      rendering_info.renderArea.extent = m_swapchain_extent;
    }
    if (old)
    {
      VK_DECLARE_PFN(dl, vkDestroySwapchainKHR);
      vkDestroySwapchainKHR(device, old, nullptr);
    }
    m_status &= ~renderer_status_flag_bits::dirty_bit;
  }

  void render_window_impl::initialize_keyboard() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    m_keyboard_map = xkb_x11_keymap_new_from_device(parent.wsi().keyboard_context(), parent.wsi().connection(),
                                                    parent.wsi().keyboard(), XKB_KEYMAP_COMPILE_NO_FLAGS);
    m_keyboard_state = xkb_x11_state_new_from_device(m_keyboard_map, parent.wsi().connection(),
                                                     parent.wsi().keyboard());
    // Map names to keycodes.
    {
#define OBERON_INTERNAL_LINUX_X11_KEYCODE_MAPPING(name, str) (str),
      const auto key_names = std::array<cstring, MAX_KEY>{ OBERON_INTERNAL_LINUX_X11_KEYCODE_MAP };
#undef OBERON_INTERNAL_LINUX_X11_KEYCODE_MAPPING
      auto cur = m_to_keycode.begin();
      for (const auto name : key_names)
      {
        *(cur++) = xkb_keymap_key_by_name(m_keyboard_map, name);
      }
    }
    // Map xcb_keycode_t to oberon::key
    {
      auto i = 1;
      for (const auto keycode : m_to_keycode)
      {
        m_to_external_key[keycode] = static_cast<oberon::key>(i++);
      }
    }
    // Map modifier names to modifier indices
    {
#define OBERON_INTERNAL_LINUX_X11_MODIFIER_KEY_MAPPING(name, str) (str),
      const auto modifier_names = std::array<cstring, MAX_MODIFIER_KEY>{ OBERON_INTERNAL_LINUX_X11_MODIFIER_KEY_MAP };
#undef OBERON_INTERNAL_LINUX_X11_MODIFIER_KEY_MAPPING
      auto cur = m_to_modifier_index.begin();
      for (const auto name : modifier_names)
      {
        *(cur++) = xkb_keymap_mod_get_index(m_keyboard_map, name);
      }
    }
  }

  void render_window_impl::deinitialize_keyboard() {
    xkb_state_unref(m_keyboard_state);
    m_keyboard_state = nullptr;
    xkb_keymap_unref(m_keyboard_map);
    m_keyboard_map = nullptr;
  }

  void render_window_impl::begin_frame() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto& dl = parent.dispatch_loader();
    const auto& device = parent.device_handle();
    VK_DECLARE_PFN(dl, vkWaitForFences);
    if (m_status & renderer_status_flag_bits::dirty_bit)
    {
      VK_SUCCEEDS(vkWaitForFences(device, m_frame_fences.size(), m_frame_fences.data(), true, VK_FOREVER));
      initialize_swapchain(m_swapchain);
      deinitialize_depth_stencil();
      initialize_depth_stencil();
    }
    VK_DECLARE_PFN(dl, vkAcquireNextImageKHR);
    {
      const auto semaphore = (m_current_frame * SEMAPHORES_PER_FRAME) + IMAGE_ACQUIRED_SEMAPHORE_INDEX;
      const auto status = vkAcquireNextImageKHR(device, m_swapchain, VK_FOREVER, m_frame_semaphores[semaphore],
                                                VK_NULL_HANDLE, &m_current_image);
      switch (status)
      {
      case VK_SUBOPTIMAL_KHR:
      case VK_ERROR_OUT_OF_DATE_KHR:
        m_status |= renderer_status_flag_bits::dirty_bit;
      case VK_SUCCESS:
        break;
      default:
        OBERON_CHECK_ERROR_MSG(false, 1, "Failed to acquire an image for rendering.");
      }
      auto attachment = (m_current_frame * ATTACHMENTS_PER_FRAME) + COLOR_ATTACHMENT_INDEX;
      auto& color_attachment = m_frame_attachments[attachment];
      color_attachment.imageView = m_swapchain_image_views[m_current_image];
    }
    VK_SUCCEEDS(vkWaitForFences(device, 1, &m_frame_fences[m_current_frame], true, VK_FOREVER));
    VK_DECLARE_PFN(dl, vkResetCommandPool);
    VK_SUCCEEDS(vkResetCommandPool(device, m_frame_command_pools[m_current_frame], 0));
    auto begin_info = VkCommandBufferBeginInfo{ };
    begin_info.sType = VK_STRUCT(COMMAND_BUFFER_BEGIN_INFO);
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    const auto graphics = (m_current_frame * COMMAND_BUFFERS_PER_FRAME) + GRAPHICS_COMMAND_BUFFER_INDEX;
    VK_DECLARE_PFN(dl, vkBeginCommandBuffer);
    VK_SUCCEEDS(vkBeginCommandBuffer(m_frame_command_buffers[graphics], &begin_info));
    VK_DECLARE_PFN(dl, vkCmdPipelineBarrier);
    auto image_memory_barriers = std::array<VkImageMemoryBarrier, 2>{ };
    {
      auto& color_barrier = image_memory_barriers[0];
      color_barrier.sType = VK_STRUCT(IMAGE_MEMORY_BARRIER);
      color_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      color_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      color_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color_barrier.srcQueueFamilyIndex = -1;
      color_barrier.dstQueueFamilyIndex = -1;
      color_barrier.image = m_swapchain_images[m_current_image];
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
      depth_stencil_barrier.image = m_frame_depth_stencil_images[m_current_frame]->image;
      depth_stencil_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      depth_stencil_barrier.subresourceRange.baseMipLevel = 0;
      depth_stencil_barrier.subresourceRange.levelCount = std::numeric_limits<u32>::max();
      depth_stencil_barrier.subresourceRange.baseArrayLayer = 0;
      depth_stencil_barrier.subresourceRange.layerCount = std::numeric_limits<u32>::max();
    }
    vkCmdPipelineBarrier(m_frame_command_buffers[graphics], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                         0, 0, nullptr, 0, nullptr, image_memory_barriers.size(), image_memory_barriers.data());
    VK_DECLARE_PFN(dl, vkCmdBeginRendering);
    // This should begin the render, set the viewport and suspend.
    // BEGIN -> LOAD_OP -> SetViewport -> SetScissor -> SUSPEND
    auto& rendering_info = m_frame_rendering_info[m_current_frame];
    rendering_info.flags = VK_RENDERING_SUSPENDING_BIT;
    vkCmdBeginRendering(m_frame_command_buffers[graphics], &rendering_info);
    auto viewport = VkViewport{ };
    viewport.x = rendering_info.renderArea.offset.x;
    viewport.y = rendering_info.renderArea.offset.y;
    viewport.width = rendering_info.renderArea.extent.width;
    viewport.height = rendering_info.renderArea.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VK_DECLARE_PFN(dl, vkCmdSetViewport);
    vkCmdSetViewport(m_frame_command_buffers[graphics], 0, 1, &viewport);
    VK_DECLARE_PFN(dl, vkCmdSetScissor);
    vkCmdSetScissor(m_frame_command_buffers[graphics], 0, 1, &rendering_info.renderArea);
    VK_DECLARE_PFN(dl, vkCmdEndRendering);
    vkCmdEndRendering(m_frame_command_buffers[graphics]);
    rendering_info.flags |= VK_RENDERING_RESUMING_BIT;
  }

  void render_window_impl::end_frame() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto& dl = parent.dispatch_loader();
    const auto graphics = (m_current_frame * COMMAND_BUFFERS_PER_FRAME) + GRAPHICS_COMMAND_BUFFER_INDEX;
    const auto image_acquired = (m_current_frame * SEMAPHORES_PER_FRAME) + IMAGE_ACQUIRED_SEMAPHORE_INDEX;
    const auto render_finished = (m_current_frame * SEMAPHORES_PER_FRAME) + RENDER_FINISHED_SEMAPHORE_INDEX;
    auto& rendering_info = m_frame_rendering_info[m_current_frame];
    rendering_info.flags = VK_RENDERING_RESUMING_BIT;
    VK_DECLARE_PFN(dl, vkCmdBeginRendering);
    // This should resume the suspended render, then store to the images, and end the render.
    // RESUME -> STORE_OP -> END
    vkCmdBeginRendering(m_frame_command_buffers[graphics], &rendering_info);
    VK_DECLARE_PFN(dl, vkCmdEndRendering);
    vkCmdEndRendering(m_frame_command_buffers[graphics]);
    auto image_memory_barriers = std::array<VkImageMemoryBarrier, 1>{ };
    {
      auto& color_barrier = image_memory_barriers[0];
      color_barrier.sType = VK_STRUCT(IMAGE_MEMORY_BARRIER);
      color_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      color_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      color_barrier.srcQueueFamilyIndex = -1;
      color_barrier.dstQueueFamilyIndex = -1;
      color_barrier.image = m_swapchain_images[m_current_image];
      color_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      color_barrier.subresourceRange.baseMipLevel = 0;
      color_barrier.subresourceRange.levelCount = std::numeric_limits<u32>::max();
      color_barrier.subresourceRange.baseArrayLayer = 0;
      color_barrier.subresourceRange.layerCount = std::numeric_limits<u32>::max();
    }
    VK_DECLARE_PFN(dl, vkCmdPipelineBarrier);
    // Transition image to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    vkCmdPipelineBarrier(m_frame_command_buffers[graphics], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr,
                         image_memory_barriers.size(), image_memory_barriers.data());
    VK_DECLARE_PFN(dl, vkEndCommandBuffer);
    VK_SUCCEEDS(vkEndCommandBuffer(m_frame_command_buffers[graphics]));
    VK_DECLARE_PFN(dl, vkQueueSubmit);
    auto submit_info = VkSubmitInfo{ };
    submit_info.sType = VK_STRUCT(SUBMIT_INFO);
    submit_info.commandBufferCount = COMMAND_BUFFERS_PER_FRAME;
    submit_info.pCommandBuffers = &m_frame_command_buffers[m_current_frame * COMMAND_BUFFERS_PER_FRAME];
    submit_info.pWaitSemaphores = &m_frame_semaphores[image_acquired];
    submit_info.waitSemaphoreCount = 1;
    auto wait_stage = VkPipelineStageFlags{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.pSignalSemaphores = &m_frame_semaphores[render_finished];
    submit_info.signalSemaphoreCount = 1;
    VK_DECLARE_PFN(dl, vkResetFences);
    VK_SUCCEEDS(vkResetFences(parent.device_handle(), 1, &m_frame_fences[m_current_frame]));
    // This is slow. Only do it once per queue per frame.
    const auto& queue = parent.queue();
    VK_SUCCEEDS(vkQueueSubmit(queue, 1, &submit_info, m_frame_fences[m_current_frame]));
    auto present_info = VkPresentInfoKHR{ };
    present_info.sType = VK_STRUCT(PRESENT_INFO_KHR);
    present_info.pSwapchains = &m_swapchain;
    present_info.swapchainCount = 1;
    present_info.pImageIndices = &m_current_image;
    present_info.pWaitSemaphores = &m_frame_semaphores[render_finished];
    present_info.waitSemaphoreCount = 1;
    VK_DECLARE_PFN(dl, vkQueuePresentKHR);
    {
      auto status = vkQueuePresentKHR(queue, &present_info);
      switch (status)
      {
      case VK_ERROR_OUT_OF_DATE_KHR:
      case VK_SUBOPTIMAL_KHR:
        m_status |= renderer_status_flag_bits::dirty_bit;
      case VK_SUCCESS:
        break;
      default:
        OBERON_CHECK_ERROR_MSG(false, 1, "Failed to present image.");
      }
    }
    // Equivalent to m_frame_index = (m_frame_index + 1) % OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT.
    m_current_frame = (m_current_frame + 1) & (FRAME_COUNT - 1);
  }

  u32 render_window_impl::id() const {
    return m_window;
  }


  void render_window_impl::show() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    xcb_map_window(parent.wsi().connection(), m_window);
    xcb_flush(parent.wsi().connection());
  }

  void render_window_impl::hide() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    xcb_unmap_window(parent.wsi().connection(), m_window);
    xcb_flush(parent.wsi().connection());
  }

  bitmask render_window_impl::query_visibility() const {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    const auto window_attrs_req = XCB_SEND_REQUEST(xcb_get_window_attributes, connection, m_window);
    const auto wm_state = parent.wsi().atom_by_name(WM_STATE_ATOM);
    const auto wm_state_req = XCB_SEND_REQUEST(xcb_get_property, connection, false, m_window, wm_state, wm_state, 0,
                                               sizeof(xcb_wm_state_t) >> 2);
    const auto net_wm_state = parent.wsi().atom_by_name(NET_WM_STATE_ATOM);
    const auto net_wm_state_req = XCB_SEND_REQUEST(xcb_get_property, connection, false, m_window, net_wm_state,
                                                   XCB_ATOM_ATOM, 0, (20 * sizeof(xcb_atom_t)) >> 2);
    auto error = ptr<xcb_generic_error_t>{ };
    auto window_attrs_rep = XCB_AWAIT_REPLY(xcb_get_window_attributes, connection, window_attrs_req, &error);
    XCB_HANDLE_ERROR(window_attrs_rep, error, "Failed to retrieve X11 window attributes.");
    auto result = bitmask{ };
    if (window_attrs_rep->map_state == XCB_MAP_STATE_VIEWABLE)
    {
      result |= query_visibility_flag_bits::mapped_bit;
    }
    std::free(window_attrs_rep);
    auto wm_state_rep = XCB_AWAIT_REPLY(xcb_get_property, connection, wm_state_req, &error);
    XCB_HANDLE_ERROR(wm_state_rep, error, "Failed to retrieve ICCCM window state.");
    if (xcb_get_property_value_length(wm_state_rep) == sizeof(xcb_wm_state_t))
    {
      const auto& wm_state_val = *reinterpret_cast<ptr<xcb_wm_state_t>>(xcb_get_property_value(wm_state_rep));
      result |= query_visibility_flag_bits::iconic_bit & -(wm_state_val.state == ICONIC_STATE);
    }
    std::free(wm_state_rep);
    auto net_wm_state_rep = XCB_AWAIT_REPLY(xcb_get_property, connection, net_wm_state_req, &error);
    XCB_HANDLE_ERROR(net_wm_state_rep, error, "Failed to retrieve EWMH window state.");
    if (xcb_get_property_value_length(net_wm_state_rep) / sizeof(xcb_atom_t) > 0)
    {
      const auto net_wm_state_hidden = parent.wsi().atom_by_name(NET_WM_STATE_HIDDEN_ATOM);
      const auto arr = reinterpret_cast<ptr<xcb_atom_t>>(xcb_get_property_value(net_wm_state_rep));
      for (auto i = 0; i < xcb_get_property_value_length(net_wm_state_rep); ++i)
      {
        result ^= query_visibility_flag_bits::hidden_bit & -(arr[i] == net_wm_state_hidden);
      }
    }
    std::free(net_wm_state_rep);
    return result;
  }

  bool render_window_impl::is_shown() const {
    const auto bits = query_visibility();
    return (bits == query_visibility_flag_bits::mapped_bit) ||
           (bits == (query_visibility_flag_bits::iconic_bit | query_visibility_flag_bits::hidden_bit));
  }

  bool render_window_impl::is_minimized() const {
    const auto bits = query_visibility();
    return bits == (query_visibility_flag_bits::iconic_bit | query_visibility_flag_bits::hidden_bit);
  }

  void render_window_impl::change_ewmh_states(const ewmh_state_action action, const xcb_atom_t first,
                                             const xcb_atom_t second) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    auto generic = xcb_generic_event_t{ };
    auto& client_message = *reinterpret_cast<ptr<xcb_client_message_event_t>>(&generic);
    client_message.response_type = XCB_CLIENT_MESSAGE;
    client_message.window = m_window;
    client_message.type = parent.wsi().atom_by_name(NET_WM_STATE_ATOM);
    client_message.format = 32;
    client_message.data.data32[0] = action;
    client_message.data.data32[1] = first;
    client_message.data.data32[2] = second;
    client_message.data.data32[3] = APPLICATION_SOURCE;
    client_message.data.data32[4] = 0;
    send_client_message(parent.wsi().default_screen()->root, generic);
  }

  void render_window_impl::change_compositor_mode(const compositor_mode mode) {
      auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
      xcb_change_property(parent.wsi().connection(), XCB_PROP_MODE_REPLACE, m_window,
                          parent.wsi().atom_by_name(NET_WM_BYPASS_COMPOSITOR_ATOM), XCB_ATOM_CARDINAL, 32, 1, &mode);
  }

  void render_window_impl::change_display_style(const display_style style) {
    if (style == current_display_style())
    {
      return;
    }
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto above = parent.wsi().atom_by_name(NET_WM_STATE_ABOVE_ATOM);
    const auto fullscreen = parent.wsi().atom_by_name(NET_WM_STATE_FULLSCREEN_ATOM);
    switch (style)
    {
    case display_style::windowed:
      {
        change_ewmh_states(REMOVE_WM_STATE_ACTION, above, fullscreen);
        change_compositor_mode(NO_PREFERENCE_COMPOSITOR);
      }
      break;
    case display_style::fullscreen_composited:
      {
        change_ewmh_states(ADD_WM_STATE_ACTION, fullscreen, XCB_NONE);
        change_compositor_mode(NO_PREFERENCE_COMPOSITOR);
      }
      break;
    case display_style::fullscreen_bypass_compositor:
      {
        change_ewmh_states(ADD_WM_STATE_ACTION, above, fullscreen);
        change_compositor_mode(DISABLE_COMPOSITOR);
      }
      break;
    default:
      break;
    }
    xcb_flush(parent.wsi().connection());
  }

  display_style render_window_impl::current_display_style() const {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    auto net_wm_state_req = XCB_SEND_REQUEST(xcb_get_property, connection, false, m_window,
                                             parent.wsi().atom_by_name(NET_WM_STATE_ATOM), XCB_ATOM_ATOM, 0,
                                             (20 * sizeof(xcb_atom_t)) >> 2);
    auto net_wm_bypass_req = XCB_SEND_REQUEST(xcb_get_property, connection, false, m_window,
                                              parent.wsi().atom_by_name(NET_WM_BYPASS_COMPOSITOR_ATOM),
                                              XCB_ATOM_CARDINAL, 0, 1);
    auto error = ptr<xcb_generic_error_t>{ };
    auto net_wm_state_rep = XCB_AWAIT_REPLY(xcb_get_property, connection, net_wm_state_req, &error);
    XCB_HANDLE_ERROR(net_wm_state_rep, error, "Failed to retrieve EWMH window state.");
    const auto atom_count = xcb_get_property_value_length(net_wm_state_rep) / sizeof(xcb_atom_t);
    const auto atoms = reinterpret_cast<readonly_ptr<xcb_atom_t>>(xcb_get_property_value(net_wm_state_rep));
    constexpr auto ABOVE = bitmask{ 0x1 };
    constexpr auto FULLSCREEN = bitmask{ 0x2 };
    constexpr auto BYPASS = bitmask{ 0x4 };
    auto flags = bitmask{ };
    const auto above_atom = parent.wsi().atom_by_name(NET_WM_STATE_ABOVE_ATOM);
    const auto fullscreen_atom = parent.wsi().atom_by_name(NET_WM_STATE_FULLSCREEN_ATOM);
    for (auto i = u32{ 0 }; i < atom_count; ++i)
    {
      flags |= ABOVE & -(atoms[i] == above_atom);
      flags |= FULLSCREEN & -(atoms[i] == fullscreen_atom);
    }
    std::free(net_wm_state_rep);
    const auto net_wm_bypass_rep = XCB_AWAIT_REPLY(xcb_get_property, connection, net_wm_bypass_req, &error);
    XCB_HANDLE_ERROR(net_wm_state_rep, error, "Failed to retrieve EWMH compositor hint.");
    if (xcb_get_property_value_length(net_wm_bypass_rep) == sizeof(u32))
    {
      const auto value = *reinterpret_cast<readonly_ptr<u32>>(xcb_get_property_value(net_wm_bypass_rep));
      flags |= BYPASS & -(value == DISABLE_COMPOSITOR);
    }
    std::free(net_wm_bypass_rep);
    switch (flags)
    {
    case (ABOVE | FULLSCREEN | BYPASS):
      return display_style::fullscreen_bypass_compositor;
    case (FULLSCREEN):
      return display_style::fullscreen_composited;
    default:
      return display_style::windowed;
    }
  }

  rect_2d render_window_impl::current_drawable_rect() const {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    auto geometry_rep = ptr<xcb_get_geometry_reply_t>{ };
    XCB_SEND_REQUEST_SYNC(geometry_rep, xcb_get_geometry, connection, m_window);
    auto result = rect_2d{ { geometry_rep->x, geometry_rep->y }, { geometry_rep->width, geometry_rep->height } };
    std::free(geometry_rep);
    const auto root = parent.wsi().default_screen()->root;
    auto translate_rep = ptr<xcb_translate_coordinates_reply_t>{ };
    XCB_SEND_REQUEST_SYNC(translate_rep, xcb_translate_coordinates, connection, m_window, root, result.offset.x,
                          result.offset.y);
    result.offset = { translate_rep->dst_x, translate_rep->dst_y };
    std::free(translate_rep);
    return result;
  }

  rect_2d render_window_impl::current_rect() const {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    // The size of 1 CARD32 value is 1 X11 word (32 bits). Therefore the length is 4.
    auto request = XCB_SEND_REQUEST(xcb_get_property, connection, false, m_window,
                                    parent.wsi().atom_by_name(NET_FRAME_EXTENTS_ATOM), XCB_ATOM_CARDINAL, 0, 4);
    auto result = current_drawable_rect();
    auto error = ptr<xcb_generic_error_t>{ };
    auto reply = XCB_AWAIT_REPLY(xcb_get_property, connection, request, &error);
    XCB_HANDLE_ERROR(reply, error, "Failed to retrieve EWMH frame extents.");
    // It's possible for this function to be issued and reach this point before _NET_FRAME_EXTENTS is set on the
    // corresponding window. As a result it's important to handle this case. If the desired atom is unset then
    // this returns the drawable region unchanged.
    // If an application wishes to retrieve the correct information just after a window is mapped then it should wait
    // for the window to be mapped and updated by the WM.
    if ((xcb_get_property_value_length(reply) >> 2) == 4)
    {
      const auto values = reinterpret_cast<readonly_ptr<u32>>(xcb_get_property_value(reply));
      result.offset.x -= values[0];
      result.offset.y -= values[2];
      result.extent.width += values[0] + values[1];
      result.extent.height += values[2] + values[3];
    }
    std::free(reply);
    return result;
  }

  void render_window_impl::send_client_message(const xcb_window_t destination, const xcb_generic_event_t& message) {
    OBERON_PRECONDITION((message.response_type & ~response_type_bits::synthetic_bit) == XCB_CLIENT_MESSAGE);
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    constexpr const auto MASK = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    xcb_send_event(parent.wsi().connection(), false, destination, MASK, reinterpret_cast<cstring>(&message));
  }

  void render_window_impl::change_title(const std::string& title) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    // The encoding of WM_NAME is ambiguous.
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(WM_NAME_ATOM),
                        XCB_ATOM_STRING, 8, title.size(), title.c_str());
    // _NET_WM_NAME should be preferred and is unambiguously UTF-8 encoded.
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(NET_WM_NAME_ATOM),
                        parent.wsi().atom_by_name(UTF8_STRING_ATOM), 8, title.size(), title.c_str());
    xcb_flush(connection);
  }

  std::string render_window_impl::title() const {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    const auto utf = parent.wsi().atom_by_name(UTF8_STRING_ATOM);
    auto reply = ptr<xcb_get_property_reply_t>{ };
    XCB_SEND_REQUEST_SYNC(reply, xcb_get_property, connection, false, m_window,
                          parent.wsi().atom_by_name(NET_WM_NAME_ATOM), utf, 0, 1024 << 2);
    if (reply->format != 8 || reply->type != utf || reply->bytes_after > 0)
    {
      std::free(reply);
      OBERON_CHECK_ERROR_MSG(false, 1, "Failed to retrieve the window name.");
    }
    auto result = std::string{ reinterpret_cast<cstring>(xcb_get_property_value(reply)) };
    std::free(reply);
    return result;
  }

  void render_window_impl::resize(const extent_2d& extent) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    auto hints = xcb_size_hints_t{ };
    hints.flags = size_hint_flag_bits::program_min_size_bit | size_hint_flag_bits::program_max_size_bit |
                  size_hint_flag_bits::user_position_bit | size_hint_flag_bits::user_size_bit;
    hints.min_width = 0;
    hints.max_width = std::numeric_limits<u16>::max();
    hints.min_height = 0;
    hints.max_height = std::numeric_limits<u16>::max();
    change_size_hints(hints);
    const auto values = std::array<u32, 2>{ extent.width, extent.height };
    xcb_configure_window(connection, m_window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values.data());
    hints.min_width = extent.width;
    hints.max_width = extent.width;
    hints.min_height = extent.height;
    hints.max_height = extent.height;
    change_size_hints(hints);
    xcb_flush(connection);
  }

  void render_window_impl::move_to(const offset_2d& offset) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    const auto values = std::array<i32, 2>{ offset.x, offset.y };
    xcb_configure_window(connection, m_window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values.data());
    xcb_flush(connection);
  }

  event render_window_impl::poll_events() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    auto protocol_message = ptr<nng_msg>{ };
    auto status = nng_recvmsg(m_socket, &protocol_message, NNG_FLAG_NONBLOCK);
    if (status == NNG_EAGAIN)
    {
      return event{ event_type::none, { } };
    }
    // This is presumptive. The worker must always submit wsi_event_messages.
    auto& message = *reinterpret_cast<ptr<wsi_event_message>>(nng_msg_body(protocol_message));
    auto result = message.data;
    switch (result.type)
    {
    case oberon::event_type::platform:
      {
        const auto generic = reinterpret_cast<ptr<xcb_generic_event_t>>(&result.data.platform.pad[0]);
        const auto type = generic->response_type & ~response_type_bits::synthetic_bit;
        if (parent.wsi().is_keyboard_event(type))
        {
          const auto xkb = reinterpret_cast<ptr<xcb_xkb_generic_event_t>>(generic);
          switch (xkb->xkb_code)
          {
          case XCB_XKB_STATE_NOTIFY:
            {
              const auto state_notify = reinterpret_cast<ptr<xcb_xkb_state_notify_event_t>>(xkb);
              const auto res = xkb_state_update_mask(m_keyboard_state, state_notify->baseMods,
                                                     state_notify->latchedMods, state_notify->lockedMods,
                                                     state_notify->baseGroup, state_notify->latchedGroup,
                                                     state_notify->lockedGroup);
              OBERON_CHECK_ERROR_MSG(res, 1, "No modifiers were altered as the result of a state notify event.");
            }
            break;
          case XCB_XKB_NEW_KEYBOARD_NOTIFY:
          case XCB_XKB_MAP_NOTIFY:
            deinitialize_keyboard();
            initialize_keyboard();
            break;
          default:
            break;
          }
        }
        switch (type)
        {
        case XCB_CLIENT_MESSAGE:
          {
            const auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(generic);
            if (client_message->type == parent.wsi().atom_by_name(WM_PROTOCOLS_ATOM) && client_message->format == 32 &&
                client_message->data.data32[0] == parent.wsi().atom_by_name(NET_WM_PING_ATOM))
            {
              auto response = xcb_generic_event_t{ };
              std::memcpy(&response, generic, sizeof(xcb_generic_event_t));
              auto& message = *reinterpret_cast<ptr<xcb_client_message_event_t>>(&response);
              const auto root = parent.wsi().default_screen()->root;
              message.window = root;
              send_client_message(root, response);
            }
          }
          break;
        }
      }
      break;
    case oberon::event_type::key_press:
      {
        const auto index = static_cast<usize>(translate_keycode(result.data.key_press.key)) - 1;
        m_key_states[index].pressed = true;
        m_key_states[index].echoing = result.data.key_press.echoing;
      }
      break;
    case oberon::event_type::key_release:
      {
        const auto index = static_cast<usize>(translate_keycode(result.data.key_press.key)) - 1;
        m_key_states[index].pressed = false;
        m_key_states[index].echoing = false;
      }
      break;
    case oberon::event_type::button_press:
      {
        const auto index = static_cast<usize>(translate_mouse_buttoncode(result.data.button_press.button)) - 1;
        m_mouse_button_states[index].pressed = true;
      }
      break;
    case oberon::event_type::button_release:
      {
        const auto index = static_cast<usize>(translate_mouse_buttoncode(result.data.button_press.button)) - 1;
        m_mouse_button_states[index].pressed = false;
      }
      break;
    case oberon::event_type::geometry_reconfigure:
      {
        if (result.data.geometry_reconfigure.geometry.extent.width != m_swapchain_extent.width ||
            result.data.geometry_reconfigure.geometry.extent.height != m_swapchain_extent.height)
        {
          m_status |= renderer_status_flag_bits::dirty_bit;
        }
      }
      break;
    default:
      break;
    }
    // Receiving the message is taking ownership of it so we need to release the message here.
    nng_msg_free(protocol_message);
    return result;
  }

  oberon::key render_window_impl::translate_keycode(const u32 code) const {
    return code >= m_to_external_key.size() ? oberon::key::none : m_to_external_key[code];
  }

  oberon::mouse_button render_window_impl::translate_mouse_buttoncode(const u32 code) const {
    return static_cast<oberon::mouse_button>(code);
  }

  bool render_window_impl::is_modifier_pressed(const oberon::modifier_key modifier) const {
    if (modifier == oberon::modifier_key::none)
    {
      return false;
    }
    const auto index = m_to_modifier_index[static_cast<usize>(modifier) - 1];
    return xkb_state_mod_index_is_active(m_keyboard_state, index, XKB_STATE_MODS_EFFECTIVE);
  }

  bool render_window_impl::is_key_pressed(const oberon::key k) const {
    if (k == oberon::key::none)
    {
      return false;
    }
    return m_key_states[static_cast<usize>(k) - 1].pressed;
  }

  bool render_window_impl::is_key_echoing(const oberon::key k) const {
    if (k == oberon::key::none)
    {
      return false;
    }
    return m_key_states[static_cast<usize>(k) - 1].echoing;
  }

  bool render_window_impl::is_mouse_button_pressed(const oberon::mouse_button mb) const {
    if (mb == oberon::mouse_button::none)
    {
      return false;
    }
    return m_mouse_button_states[static_cast<usize>(mb) - 1].pressed;
  }

  void render_window_impl::draw_test_image() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto& dl = parent.dispatch_loader();
    const auto index = (m_current_frame * COMMAND_BUFFERS_PER_FRAME) + GRAPHICS_COMMAND_BUFFER_INDEX;
    auto& commands = m_frame_command_buffers[index];
    auto& rendering_info = m_frame_rendering_info[m_current_frame];
    VK_DECLARE_PFN(dl, vkCmdBeginRendering);
    vkCmdBeginRendering(commands, &rendering_info);
    VK_DECLARE_PFN(dl, vkCmdBindPipeline);
    vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_test_image_pipeline);
    VK_DECLARE_PFN(dl, vkCmdDraw);
    vkCmdDraw(commands, 3, 1, 0, 0);
    VK_DECLARE_PFN(dl, vkCmdEndRendering);
    vkCmdEndRendering(commands);
  }

  void render_window_impl::swap_buffers() {
    end_frame();
    begin_frame();
  }

  const std::unordered_set<presentation_mode>& render_window_impl::available_presentation_modes() const {
    return m_presentation_modes;
  }

  void render_window_impl::request_presentation_mode(const presentation_mode mode) {
    if (m_presentation_modes.contains(mode))
    {
      m_swapchain_present_mode = static_cast<VkPresentModeKHR>(mode);
    }
    else
    {
      m_swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }
    m_status |= renderer_status_flag_bits::dirty_bit;
  }

  presentation_mode render_window_impl::current_presentation_mode() const {
    return static_cast<presentation_mode>(m_swapchain_present_mode);
  }

  void render_window_impl::copy_buffer(VkBuffer from, VkBuffer to, const u32 size) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    VK_DECLARE_PFN(parent.dispatch_loader(), vkCmdCopyBuffer);
    auto region = VkBufferCopy{ };
    region.size = size;
    region.dstOffset = 0;
    region.srcOffset = 0;
    vkCmdCopyBuffer(m_frame_command_buffers[m_current_frame], from, to, 1, &region);
  }

  void render_window_impl::insert_memory_barrier(const VkMemoryBarrier& barrier, const VkPipelineStageFlags src,
                                                 const VkPipelineStageFlags dest) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    VK_DECLARE_PFN(parent.dispatch_loader(), vkCmdPipelineBarrier);
    vkCmdPipelineBarrier(m_frame_command_buffers[m_current_frame], src, dest, 0, 1, &barrier, 0, nullptr, 0, nullptr);
  }

  void render_window_impl::change_active_camera(camera& cam) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    auto& impl = cam.implementation();
    if (m_active_camera)
    {
      m_active_camera->detach_window(m_active_camera_cookie);
    }
    m_active_camera = &impl;
    m_active_camera_cookie = impl.attach_window(*this);
    auto buffer_info = VkDescriptorBufferInfo{ };
    buffer_info.buffer = impl.resident_buffer();
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;
    auto write_descriptors = std::array<VkWriteDescriptorSet, FRAME_COUNT>{ };
    auto i = usize{ 0 };
    for (auto& write_descriptor : write_descriptors)
    {
      const auto index = ((i++) * DESCRIPTOR_SETS_PER_FRAME) + CAMERA_DESCRIPTOR_SET_INDEX;
      write_descriptor.sType = VK_STRUCT(WRITE_DESCRIPTOR_SET);
      write_descriptor.dstSet = m_frame_descriptor_sets[index];
      write_descriptor.dstBinding = 0;
      write_descriptor.dstArrayElement = 0;
      write_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      write_descriptor.descriptorCount = 1;
      write_descriptor.pBufferInfo = &buffer_info;
    }
    VK_DECLARE_PFN(parent.dispatch_loader(), vkUpdateDescriptorSets);
    vkUpdateDescriptorSets(parent.device_handle(), write_descriptors.size(), write_descriptors.data(), 0, nullptr);
  }

  void render_window_impl::draw(mesh& m) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    auto& impl = m.implementation();
    const auto& dl = parent.dispatch_loader();
    const auto index = (m_current_frame * COMMAND_BUFFERS_PER_FRAME) + GRAPHICS_COMMAND_BUFFER_INDEX;
    auto& commands = m_frame_command_buffers[index];
    impl.flush_to_device(*this);
    auto write_descriptor = VkWriteDescriptorSet{ };
    write_descriptor.sType = VK_STRUCT(WRITE_DESCRIPTOR_SET);
    const auto descriptor_set = (m_current_frame * DESCRIPTOR_SETS_PER_FRAME) + MESH_DESCRIPTOR_SET_INDEX;
    write_descriptor.dstSet = m_frame_descriptor_sets[descriptor_set];
    write_descriptor.dstBinding = 0;
    write_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor.descriptorCount = 1;
    auto buffer_info = VkDescriptorBufferInfo{ };
    buffer_info.buffer = impl.uniform_resident_buffer();
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;
    write_descriptor.pBufferInfo = &buffer_info;
    VK_DECLARE_PFN(dl, vkUpdateDescriptorSets);
    vkUpdateDescriptorSets(parent.device_handle(), 1, &write_descriptor, 0, nullptr);
    auto& rendering_info = m_frame_rendering_info[m_current_frame];
    VK_DECLARE_PFN(dl, vkCmdBeginRendering);
    vkCmdBeginRendering(commands, &rendering_info);
    VK_DECLARE_PFN(dl, vkCmdBindPipeline);
    VK_DECLARE_PFN(dl, vkCmdBindDescriptorSets);
    switch (impl.type())
    {
    case vertex_type::position_color:
      vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_unlit_pc_pipeline_layout, 0, 2,
                              &m_frame_descriptor_sets[m_current_frame * DESCRIPTOR_SETS_PER_FRAME], 0, nullptr);
      vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_unlit_pc_pipeline);
      break;
    default:
      OBERON_CHECK_ERROR_MSG(false, 1, "Failed to draw a mesh. The vertex_type was unsupported.");
    }
    VK_DECLARE_PFN(dl, vkCmdBindVertexBuffers);
    auto offset = VkDeviceSize{ 0 };
    const auto buffer = impl.resident_buffer();
    vkCmdBindVertexBuffers(commands, 0, 1, &buffer, &offset);
    VK_DECLARE_PFN(dl, vkCmdDraw);
    vkCmdDraw(commands, impl.size(), 1, 0, 0);
    VK_DECLARE_PFN(dl, vkCmdEndRendering);
    vkCmdEndRendering(commands);
  }

  void render_window_impl::clear_active_camera() {
    if (!m_active_camera)
    {
      return;
    }
    m_active_camera = nullptr;
  }

}
