#ifndef OBERON_LINUX_CONTEXT_HPP
#define OBERON_LINUX_CONTEXT_HPP

#include "../types.hpp"
#include "../memory.hpp"
#include "../errors.hpp"
#include "../context.hpp"

#include "x11_vulkan.hpp"

namespace oberon::linux {
  OBERON_EXCEPTION_TYPE(x_connection_failed, "Failed to connect to X server.", 1);
  OBERON_EXCEPTION_TYPE(x_no_screen, "Failed to find desired X screen.", 1);
  OBERON_EXCEPTION_TYPE(vulkan_instance_create_failed, "Failed to create Vulkan instance.", 1);
  OBERON_EXCEPTION_TYPE(vulkan_debug_messenger_create_failed, "Failed to create Vulkan debug messenger", 1);
  OBERON_EXCEPTION_TYPE(vulkan_couldnt_enumerate_physical_devices, "Failed to enumerate Vulkan physical devices.", 1);
  OBERON_EXCEPTION_TYPE(vulkan_device_create_failed, "Failed to create Vulkan device.", 1);

  class onscreen_context final : public oberon::onscreen_context {
  private:
    ptr<xcb_connection_t> m_x_connection{ };
    ptr<xcb_screen_t> m_x_screen{ };

    vkfl::loader m_vulkan_dl{ vkGetInstanceProcAddr };
    VkInstance m_vulkan_instance{ };
    VkDebugUtilsMessengerEXT m_vulkan_debug_messenger{ };
    VkPhysicalDevice m_vulkan_physical_device{ };
    VkDevice m_vulkan_device{ };
    VkQueue m_vulkan_work_queue{ };
    VkQueue m_vulkan_present_queue{ };

    void connect_to_x_server(const cstring displayname);
    void create_vulkan_instance(const readonly_ptr<cstring> layers, const u32 layer_count,
                                const readonly_ptr<cstring> extensions, const u32 extension_count,
                                const ptr<void> next);
    void create_vulkan_debug_messenger(const VkDebugUtilsMessengerCreateInfoEXT& debug_info);
    void select_vulkan_physical_device(const u32 device_index);
    void create_vulkan_device(const readonly_ptr<cstring> extensions, const u32 extension_count, const ptr<void> next);
    void create_vulkan_device_nvidia(VkDeviceCreateInfo& device_info);
    void create_vulkan_device_amd(VkDeviceCreateInfo& device_info);
    void create_vulkan_device_intel(VkDeviceCreateInfo& device_info);
    void create_vulkan_device_generic(VkDeviceCreateInfo& device_info);

    void destroy_vulkan_device() noexcept;
    void destroy_vulkan_debug_messenger() noexcept;
    void destroy_vulkan_instance() noexcept;
    void disconnect_from_x_server() noexcept;
  public:
    onscreen_context(const cstring x_displayname, const u32 vulkan_device_index,
                     const readonly_ptr<cstring> vulkan_layers, const u32 vulkan_layer_count,
                     const bool vulkan_enable_debug_messenger);

    ~onscreen_context() noexcept;
  };

}

#endif
