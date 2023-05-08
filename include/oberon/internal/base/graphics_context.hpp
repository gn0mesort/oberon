#ifndef OBERON_INTERNAL_BASE_GRAPHICS_CONTEXT_HPP
#define OBERON_INTERNAL_BASE_GRAPHICS_CONTEXT_HPP

#include <vector>
#include <string>
#include <unordered_set>

#include "../../memory.hpp"

#include "vulkan.hpp"

namespace oberon::internal::base {

  class graphics_context {
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
    VkInstance m_instance{ };
    std::vector<VkPhysicalDevice> m_physical_devices{ };

    graphics_context(const defer_construction&);
  public:
    graphics_context(const std::string& application_name, const u32 application_version,
                     const std::string& engine_name, const u32 engine_version,
                     const std::unordered_set<std::string>& requested_layers,
                     const std::unordered_set<std::string>& required_extensions,
                     const std::unordered_set<std::string>& requested_extensions);
    graphics_context(const graphics_context& other) = delete;
    graphics_context(graphics_context&& other) = delete;

    virtual ~graphics_context() noexcept;

    graphics_context& operator=(const graphics_context& rhs) = delete;
    graphics_context& operator=(graphics_context&& rhs) = delete;

    VkInstance instance();
    const vkfl::loader& dispatch_loader();
    const std::vector<VkPhysicalDevice>& physical_devices();
  };

  class debug_graphics_context final : public graphics_context {
  private:
    VkDebugUtilsMessengerEXT m_debug_messenger{ };
  public:
    debug_graphics_context(const std::string& application_name, const u32 application_version,
                           const std::string& engine_name, const u32 engine_version,
                           const std::unordered_set<std::string>& requested_layers,
                           const std::unordered_set<std::string>& required_extensions,
                           const std::unordered_set<std::string>& requested_extensions);
    debug_graphics_context(const debug_graphics_context& other) = delete;
    debug_graphics_context(debug_graphics_context&& other) = delete;

    ~debug_graphics_context() noexcept;

    debug_graphics_context& operator=(const debug_graphics_context& rhs) = delete;
    debug_graphics_context& operator=(debug_graphics_context&& rhs) = delete;
  };

}

#endif
