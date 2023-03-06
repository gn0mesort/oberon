/**
 * @file system.cpp
 * @brief System class implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/linux/system.hpp"

#include <cstring>
#include <cstdlib>

#include <unordered_set>

#include "oberon/debug.hpp"

#include "oberon/linux/input.hpp"
#include "oberon/linux/window.hpp"

#define OBERON_SYSTEM_X11_PRECONDITIONS \
  OBERON_PRECONDITION(m_x_display); \
  OBERON_PRECONDITION(m_x_connection); \
  OBERON_PRECONDITION(m_x_screen)

#define OBERON_SYSTEM_VK_PRECONDITIONS \
  OBERON_PRECONDITION(m_vk_instance != VK_NULL_HANDLE)

#define OBERON_SYSTEM_PRECONDITIONS \
  OBERON_SYSTEM_X11_PRECONDITIONS; \
  OBERON_SYSTEM_VK_PRECONDITIONS

namespace oberon::linux {

  system::system(const std::string& instance_name, const std::string& application_name,
                 const std::vector<std::string>& desired_layers) :
  m_instance_name{ instance_name },
  m_application_name{ application_name } {
    // Initialize X11
    {
      XInitThreads();
      m_x_display = XOpenDisplay(nullptr);
      XSetEventQueueOwner(m_x_display, XCBOwnsEventQueue);
      m_x_connection = XGetXCBConnection(m_x_display);
      OBERON_CHECK(!xcb_connection_has_error(m_x_connection));
      auto screenp = XDefaultScreen(m_x_display);
      auto setup = xcb_get_setup(m_x_connection);
      for (auto roots = xcb_setup_roots_iterator(setup); roots.rem; xcb_screen_next(&roots))
      {
        if (!(screenp--))
        {
          m_x_screen = roots.data;
        }
      }
      // Initialize XInput
      // Not the Microsoft gamepad thing. The X11 extension
      {
        auto xinput = xcb_get_extension_data(m_x_connection, &xcb_input_id);
        OBERON_CHECK_ERROR_MSG(xinput->present, 1, "The XInput extension is not available.");
        {
          auto request = xcb_input_xi_query_version(m_x_connection, 2, 0);
          auto reply = ptr<xcb_input_xi_query_version_reply_t>{ };
          OBERON_LINUX_X_SUCCEEDS(reply, xcb_input_xi_query_version_reply(m_x_connection, request, err));
          auto major = reply->major_version;
          auto minor = reply->minor_version;
          std::free(reply);
          OBERON_CHECK_ERROR_MSG(major == 2 && minor >= 0, 1, "XInput 2 is not supported.");
        }
        m_xi_major_opcode = xinput->major_opcode;
        {
          auto request = xcb_input_xi_query_device(m_x_connection, XCB_INPUT_DEVICE_ALL_MASTER);
          auto reply = ptr<xcb_input_xi_query_device_reply_t>{ };
          OBERON_LINUX_X_SUCCEEDS(reply, xcb_input_xi_query_device_reply(m_x_connection, request, err));
          auto kbd_found = false;
          auto ptr_found = false;
          for (auto itr = xcb_input_xi_query_device_infos_iterator(reply); itr.rem;
               xcb_input_xi_device_info_next(&itr))
          {
            if (!kbd_found && itr.data->enabled && itr.data->type == XCB_INPUT_DEVICE_TYPE_MASTER_KEYBOARD)
            {
              m_xi_master_keyboard_id = itr.data->deviceid;
            }
            if (!ptr_found && itr.data->enabled && itr.data->type == XCB_INPUT_DEVICE_TYPE_MASTER_POINTER)
            {
              m_xi_master_pointer_id = itr.data->deviceid;
            }
          }
          std::free(reply);
        }
      }
      {
        xkb_x11_setup_xkb_extension(m_x_connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
                                    XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr, &m_xkb_first_event,
                                    nullptr);
        // m_xkb_keyboard = xkb_x11_get_core_keyboard_device_id(m_x_connection);
        // std::printf("XKB Keyboard = %hu\nXI Keyboard = %hu\n", m_xkb_keyboard, m_xi_master_keyboard_id);
        OBERON_CHECK(m_xi_master_keyboard_id >= 0);
        // No XKB events are actually selected at this point.
        // Inexplicable code per libxkbcommon
        // (https://github.com/xkbcommon/libxkbcommon/blob/master/tools/interactive-x11.c)
        constexpr const auto required_events = XCB_XKB_EVENT_TYPE_MAP_NOTIFY | XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
                                               XCB_XKB_EVENT_TYPE_STATE_NOTIFY;
        constexpr const auto map_parts = XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS |
                                         XCB_XKB_MAP_PART_MODIFIER_MAP | XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
                                         XCB_XKB_MAP_PART_KEY_ACTIONS | XCB_XKB_MAP_PART_VIRTUAL_MODS |
                                         XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;
        constexpr const auto state_details = XCB_XKB_STATE_PART_MODIFIER_BASE | XCB_XKB_STATE_PART_MODIFIER_LATCH |
                                             XCB_XKB_STATE_PART_MODIFIER_LOCK | XCB_XKB_STATE_PART_GROUP_BASE |
                                             XCB_XKB_STATE_PART_GROUP_LATCH | XCB_XKB_STATE_PART_GROUP_LOCK;
        auto details = xcb_xkb_select_events_details_t{ };
        details.affectNewKeyboard = XCB_XKB_NKN_DETAIL_KEYCODES;
        details.newKeyboardDetails = XCB_XKB_NKN_DETAIL_KEYCODES;
        details.affectState = state_details;
        details.stateDetails = state_details;
        xcb_xkb_select_events(m_x_connection, m_xi_master_keyboard_id, required_events, 0, 0, map_parts, map_parts,
                                  &details);
        m_xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        {
          constexpr const auto mask = XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT;
          auto request = xcb_xkb_per_client_flags(m_x_connection, m_xi_master_keyboard_id, mask, mask, 0, 0, 0);
          auto reply = ptr<xcb_xkb_per_client_flags_reply_t>{ };
          OBERON_LINUX_X_SUCCEEDS(reply, xcb_xkb_per_client_flags_reply(m_x_connection, request, err));
          OBERON_CHECK(reply->supported & mask);
          OBERON_CHECK(reply->value & mask);
          std::free(reply);
        }
      }
    }
    OBERON_ASSERT(m_x_screen);
    OBERON_ASSERT(m_x_connection);
    OBERON_ASSERT(m_x_display);
    // Intern X11 atoms
    {
#define OBERON_LINUX_X_ATOM_NAME(name, str) (str),
      auto atom_names = std::array<cstring, OBERON_LINUX_X_ATOM_MAX>{ OBERON_LINUX_X_ATOMS };
#undef OBERON_LINUX_X_ATOM_NAME
      auto atom_requests = std::array<xcb_intern_atom_cookie_t, OBERON_LINUX_X_ATOM_MAX>{ };
      {
        auto cur = atom_requests.begin();
        for (const auto& atom_name : atom_names)
        {
          *(cur++) = begin_intern_atom(atom_name);
        }
      }
      {
        auto app_info = VkApplicationInfo{ };
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        // I know this is obtuse. The "application name" in the X11 sense is the name of the underlying software
        // application. The "instance name" is the name of the particular instance. While this might not always be
        // appropriate (e.g., for a text editor these might be something like Kate and Editor respectively) it's
        // probably fine here. The application should always be "oberon" and the instance should be whatever the name
        // of the application is at runtime (e.g., via argv[0], -name, or RESOURCE_NAME).
        //
        // Broadly, these settings don't matter anyway. They exist so that the driver can provide software-specific
        // optimizations or fixes. Doom or Unreal Engine might get that kind of treatment but I surely won't.
        app_info.pEngineName = m_application_name.c_str();
        app_info.pApplicationName = m_instance_name.c_str();
        // There's no facility for changing this right now but there probably should be.
        // TODO: make these variable.
        app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        // This is the highest Vulkan version that the application will support.
        app_info.apiVersion = VK_API_VERSION_1_3;
        auto instance_info = VkInstanceCreateInfo{ };
        instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_info.pApplicationInfo = &app_info;
        // Enumerate available Vulkan layers.
        auto available_layers = std::unordered_set<std::string>{ };
        {
          OBERON_LINUX_VK_DECLARE_PFN(m_vkdl, vkEnumerateInstanceLayerProperties);
          // Vulkan likes to use 32-bit sizes by the way.
          auto sz = u32{ };
          OBERON_LINUX_VK_SUCCEEDS(vkEnumerateInstanceLayerProperties(&sz, nullptr));
          auto layer_properties = std::vector<VkLayerProperties>(sz);
          OBERON_LINUX_VK_SUCCEEDS(vkEnumerateInstanceLayerProperties(&sz, layer_properties.data()));
          for (const auto& layer_property : layer_properties)
          {
            available_layers.insert(layer_property.layerName);
          }
        }
        // Select desired Vulkan layers.
        auto selected_layers = std::vector<cstring>{ };
        {
          for (const auto& desired_layer : desired_layers)
          {
            if (available_layers.contains(desired_layer))
            {
              selected_layers.emplace_back(desired_layer.c_str());
            }
          }
        }
        // Enumerate available Vulkan extensions.
        // This includes searching the selected layers for extensions.
        auto available_extensions = std::unordered_set<std::string>{ };
        {
          OBERON_LINUX_VK_DECLARE_PFN(m_vkdl, vkEnumerateInstanceExtensionProperties);
          auto sz = u32{ };
          // Enumerate extensions provided by the implementation.
          OBERON_LINUX_VK_SUCCEEDS(vkEnumerateInstanceExtensionProperties(nullptr, &sz, nullptr));
          auto extension_properties = std::vector<VkExtensionProperties>(sz);
          OBERON_LINUX_VK_SUCCEEDS(vkEnumerateInstanceExtensionProperties(nullptr, &sz, extension_properties.data()));
          for (const auto& extension_property : extension_properties)
          {
            available_extensions.insert(extension_property.extensionName);
          }
          // Enumerate the extensions provided by each selected layer.
          for (const auto& selected_layer : selected_layers)
          {
            OBERON_LINUX_VK_SUCCEEDS(vkEnumerateInstanceExtensionProperties(selected_layer, &sz, nullptr));
            extension_properties.resize(sz);
            OBERON_LINUX_VK_SUCCEEDS(vkEnumerateInstanceExtensionProperties(selected_layer, &sz,
                                                                            extension_properties.data()));
            for (const auto& extension_property : extension_properties)
            {
              available_extensions.insert(extension_property.extensionName);
            }
          }
        }
        // Select desired extensions.
        // Currently, I think it's easiest to just enable every available extension. Selecting specific extensions
        // complicates this.
        auto selected_extensions = std::vector<cstring>{ };
        {
#define OBERON_LINUX_VK_REQUIRE_EXTENSION(ext) \
  do \
  { \
    if (!available_extensions.contains((ext))) \
    { \
      throw vk_error { "The Vulkan instance does not support \"" ext "\"", 1 }; \
    } \
  } \
  while (0)
          OBERON_LINUX_VK_REQUIRE_EXTENSION(VK_KHR_SURFACE_EXTENSION_NAME);
          OBERON_LINUX_VK_REQUIRE_EXTENSION(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#ifndef NDEBUG
          OBERON_LINUX_VK_REQUIRE_EXTENSION(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
          OBERON_LINUX_VK_REQUIRE_EXTENSION(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
#endif
#undef OBERON_LINUX_VK_REQUIRE_EXTENSION
          for (const auto& extension : available_extensions)
          {
            selected_extensions.emplace_back(extension.c_str());
          }
        }
#ifndef NDEBUG
        auto enabled_validation_features = std::array<VkValidationFeatureEnableEXT, 4> {
          VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
          VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
          VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
          VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
        };
        auto validation_features = VkValidationFeaturesEXT{ };
        validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        validation_features.pEnabledValidationFeatures = enabled_validation_features.data();
        validation_features.enabledValidationFeatureCount = enabled_validation_features.size();
        auto debug_utils_info = VkDebugUtilsMessengerCreateInfoEXT{ };
        debug_utils_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_utils_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_utils_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_utils_info.pfnUserCallback = vk_debug_log;
        validation_features.pNext = &debug_utils_info;
        instance_info.pNext = &validation_features;
#endif
        instance_info.ppEnabledLayerNames = selected_layers.data();
        instance_info.enabledLayerCount = selected_layers.size();
        instance_info.ppEnabledExtensionNames = selected_extensions.data();
        instance_info.enabledExtensionCount = selected_extensions.size();
        OBERON_LINUX_VK_DECLARE_PFN(m_vkdl, vkCreateInstance);
        OBERON_LINUX_VK_SUCCEEDS(vkCreateInstance(&instance_info, nullptr, &m_vk_instance));
        OBERON_ASSERT(m_vk_instance != VK_NULL_HANDLE);
        m_vkdl.load(m_vk_instance);
#ifndef NDEBUG
        OBERON_LINUX_VK_DECLARE_PFN(m_vkdl, vkCreateDebugUtilsMessengerEXT);
        OBERON_LINUX_VK_SUCCEEDS(vkCreateDebugUtilsMessengerEXT(m_vk_instance, &debug_utils_info, nullptr,
                                                                &m_vk_debug_messenger));
        OBERON_ASSERT(m_vk_debug_messenger != VK_NULL_HANDLE);
#endif
      }
      {
        auto cur = m_x_atoms.begin();
        for (const auto& request : atom_requests)
        {
          *(cur++) = end_intern_atom(request);
        }
      }
    }
  }

  xcb_intern_atom_cookie_t system::begin_intern_atom(const cstring name) {
    OBERON_SYSTEM_X11_PRECONDITIONS;
    OBERON_PRECONDITION(name);
    return xcb_intern_atom(m_x_connection, false, std::strlen(name), name);
  }

  xcb_atom_t system::end_intern_atom(const xcb_intern_atom_cookie_t request) {
    OBERON_SYSTEM_X11_PRECONDITIONS;
    auto reply = ptr<xcb_intern_atom_reply_t>{ };
    OBERON_LINUX_X_SUCCEEDS(reply, xcb_intern_atom_reply(m_x_connection, request, err));
    auto result = reply->atom;
    std::free(reply);
    return result;
  }


  system::~system() noexcept {
    OBERON_SYSTEM_PRECONDITIONS;
#ifndef NDEBUG
    OBERON_LINUX_VK_DECLARE_PFN(m_vkdl, vkDestroyDebugUtilsMessengerEXT);
    vkDestroyDebugUtilsMessengerEXT(m_vk_instance, m_vk_debug_messenger, nullptr);
#endif
    OBERON_LINUX_VK_DECLARE_PFN(m_vkdl, vkDestroyInstance);
    vkDestroyInstance(m_vk_instance, nullptr);
    xkb_context_unref(m_xkb_context);
    XCloseDisplay(m_x_display);
  }

  std::string system::instance_name() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_instance_name;
  }

  std::string system::application_name() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_application_name;
  }

  ptr<xcb_connection_t> system::connection() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_connection;
  }

  ptr<xcb_screen_t> system::default_screen() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_screen;
  }

  xcb_window_t system::root_window_id() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_screen->root;
  }

  xcb_atom_t system::atom_from_name(const x_atom_name name) const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_atoms[name];
  }


  ptr<xkb_context> system::keyboard_context() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xkb_context;
  }

  xcb_input_device_id_t system::keyboard() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xi_master_keyboard_id;
  }

  xcb_input_device_id_t system::pointer() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xi_master_pointer_id;
  }

  u8 system::xkb_event_code() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xkb_first_event;
  }

  u8 system::xi_major_opcode() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xi_major_opcode;
  }

  VkInstance system::instance() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_vk_instance;
  }

  const vkfl::loader& system::vk_dl() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_vkdl;
  }

}
