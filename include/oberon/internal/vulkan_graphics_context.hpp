#ifndef OBERON_INTERNAL_VULKAN_GRAPHICS_CONTEXT_HPP
#define OBERON_INTERNAL_VULKAN_GRAPHICS_CONTEXT_HPP

#include <string>
#include <vector>
#include <unordered_set>

#include "../types.hpp"
#include "../memory.hpp"

#include "vulkan.hpp"

namespace oberon::internal {

  class vulkan_graphics_context {
  protected:
    static void check_instance_version(const vkfl::loader& dl);
    static VkApplicationInfo pack_application_info(const cstring application_name, const u32 application_version,
                                                   const cstring engine_name, const u32 engine_version);
    static std::unordered_set<std::string> available_layers(const vkfl::loader& dl);
    static std::vector<cstring> select_layers(std::unordered_set<std::string>& available_layers,
                                              const std::unordered_set<std::string>& requested_layers);
    static std::unordered_set<std::string> available_extensions(const vkfl::loader& dl,
                                                                std::vector<cstring>& selected_layers);
    static std::vector<cstring> select_extensions(std::unordered_set<std::string>& available_extensions,
                                                  const std::unordered_set<std::string>& required_extensions,
                                                  const std::unordered_set<std::string>& requested_extensions);
    static VkInstanceCreateInfo pack_instance_info(const readonly_ptr<VkApplicationInfo> application_info,
                                                   const ptr<void> next, const readonly_ptr<cstring> layers,
                                                   const u32 layer_count, const readonly_ptr<cstring> extensions,
                                                   const u32 extension_count);
    static void intern_instance_in_loader(vkfl::loader& dl, const VkInstance instance);
    struct defer_construction final { };

    vkfl::loader m_dl{ vkGetInstanceProcAddr };

    vulkan_graphics_context(const defer_construction&);
  public:
    vulkan_graphics_context(const std::unordered_set<std::string>& requested_layers,
                            const std::unordered_set<std::string>& required_extensions,
                            const std::unordered_set<std::string>& requested_extensions);
    vulkan_graphics_context(const vulkan_graphics_context& other) = delete;
    vulkan_graphics_context(vulkan_graphics_context&& other) = delete;

    virtual ~vulkan_graphics_context() noexcept;

    vulkan_graphics_context& operator=(const vulkan_graphics_context& rhs) = delete;
    vulkan_graphics_context& operator=(vulkan_graphics_context&& rhs) = delete;

    VkInstance instance();
    const vkfl::loader& dispatch_loader();
  };

  class debug_vulkan_graphics_context final : public vulkan_graphics_context {
  private:
    VkDebugUtilsMessengerEXT m_debug_messenger{ };
  public:
    debug_vulkan_graphics_context(const std::unordered_set<std::string>& requested_layers,
                                  const std::unordered_set<std::string>& required_extensions,
                                  const std::unordered_set<std::string>& requested_extensions);
    debug_vulkan_graphics_context(const debug_vulkan_graphics_context& other) = delete;
    debug_vulkan_graphics_context(debug_vulkan_graphics_context&& other) = delete;

    ~debug_vulkan_graphics_context() noexcept;

    debug_vulkan_graphics_context& operator=(const debug_vulkan_graphics_context& rhs) = delete;
    debug_vulkan_graphics_context& operator=(debug_vulkan_graphics_context&& rhs) = delete;
  };

}

#endif
