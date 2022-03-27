#ifndef OBERON_LINUX_CONTEXT_HPP
#define OBERON_LINUX_CONTEXT_HPP

#include <xcb/xcb.h>

#define VK_USE_PLATFORM_XCB_KHR 1
#define VK_NO_PROTOTYPES 1
#include "vkfl.hpp"
#include <vulkan/vulkan.h>

#define OBERON_DECLARE_VK_PFN(dl, cmd) \
  auto vk##cmd = (reinterpret_cast<PFN_vk##cmd>((dl).get(vkfl::command::cmd)))


#define OBERON_VK_STRUCT(name) VK_STRUCTURE_TYPE_##name

// Used by default in the vkfl loader.
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance, const char*);
extern "C" VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* pUserData);

#include "../types.hpp"
#include "../memory.hpp"

namespace oberon {

  struct x_configuration final {
    cstring displayname{ };
  };

  struct vulkan_configuration final {
    readonly_ptr<cstring> layers{ };
    u32 layer_count{ };
    bool require_debug_messenger{ };
    u32 device_index{ };
  };

  class context {
  private:
    void connect_to_x_server(const cstring displayname);
    void create_vulkan_instance(const readonly_ptr<cstring> layers, const u32 layer_count,
                                const readonly_ptr<cstring> extensions, const u32 extension_count,
                                const ptr<void> next);
    void create_vulkan_debug_messenger(const VkDebugUtilsMessengerCreateInfoEXT& debug_info);

    void destroy_vulkan_debug_messenger() noexcept;
    void destroy_vulkan_instance() noexcept;
    void disconnect_from_x_server() noexcept;
  protected:
    friend class system;

    ptr<xcb_connection_t> m_x_connection{ };
    ptr<xcb_screen_t> m_x_screen{ };

    vkfl::loader m_vulkan_dl{ vkGetInstanceProcAddr };
    VkInstance m_vulkan_instance{ };
    VkDebugUtilsMessengerEXT m_vulkan_debug_messenger{ };
    VkPhysicalDevice m_vulkan_physical_device{ };
    VkDevice m_vulkan_device{ };

    context() = default;
    context(const x_configuration& x_conf, const vulkan_configuration& vulkan_conf);

  public:
    context(const context& other) = delete;
    context(context&& other) = delete;

    virtual ~context() noexcept;

    context& operator=(const context& rhs) = delete;
    context& operator=(context&& rhs) = delete;

    ptr<xcb_connection_t> x_connection();
    ptr<xcb_screen_t> x_screen();

    VkInstance vulkan_instance();
    VkPhysicalDevice vulkan_physical_device();
    VkDevice vulkan_device();
  };

}

#endif
