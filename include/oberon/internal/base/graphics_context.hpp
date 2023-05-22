/**
 * @file graphics_context.hpp
 * @brief Internal graphics_context objects.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_BASE_GRAPHICS_CONTEXT_HPP
#define OBERON_INTERNAL_BASE_GRAPHICS_CONTEXT_HPP

#include <vector>
#include <string>
#include <unordered_set>

#include "../../memory.hpp"

#include "vulkan.hpp"

namespace oberon::internal::base {

  /**
   * @class graphics_context
   * @brief An object representing a `VkInstance` and other instance-level functionality.
   */
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
    /**
     * @brief Create a `graphics_context`
     * @param application_name The application's name. This is passed to Vulkan but probably does nothing.
     * @param application_version The application's version as if generated by `VK_MAKE_API_VERSION`. This is
     *                            passed to Vulkan but probably does nothing.
     * @param engine_name The engine's name. This is passed to Vulkan but probably does nothing.
     * @param engine_version The engine's version as if generated by `VK_MAKE_API_VERSION`. This is passed to Vulkan
     *                       but probably does nothing.
     * @param requested_layers A set of requested Vulkan layers to initialize along with the `VkInstance`.
     * @param required_extensions A set of required Vulkan instance-level extensions. If any of these extensions are
     *                            unsupported then an error will be thrown.
     * @param requested_extensions A set of requested Vulkan instance-level extensions. Any unsupported extension in
     *                             this set will simply be ignored.
     */
    graphics_context(const std::string& application_name, const u32 application_version,
                     const std::string& engine_name, const u32 engine_version,
                     const std::unordered_set<std::string>& requested_layers,
                     const std::unordered_set<std::string>& required_extensions,
                     const std::unordered_set<std::string>& requested_extensions);
    /// @cond
    graphics_context(const graphics_context& other) = delete;
    graphics_context(graphics_context&& other) = delete;
    /// @cond

    /**
     * @brief Destroy a `graphics_context`.
     */
    virtual ~graphics_context() noexcept;

    /// @cond
    graphics_context& operator=(const graphics_context& rhs) = delete;
    graphics_context& operator=(graphics_context&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve the `VkInstance` that this `graphics_context` represents.
     * @return The `VkInstance` corresponding to this `graphics_context`.
     */
    VkInstance instance_handle();

    /**
     * @brief Retrieve the `graphics_context`'s Vulkan function pointer loader and dispatch table.
     * @return A dispatch table and loader initialized with instance-level functionality.
     */
    const vkfl::loader& dispatch_loader();

    /**
     * @brief Retrieve a list of `VkPhysicalDevice`s available to the `graphics_context`.
     * @return A list of available `VkPhysicalDevice`s.
     */
    const std::vector<VkPhysicalDevice>& physical_devices();
  };

  class debug_graphics_context final : public graphics_context {
  private:
    VkDebugUtilsMessengerEXT m_debug_messenger{ };
  public:
    /**
     * @brief Create a `debug_graphics_context`.
     * @details A `debug_graphics_context` will initialize a `VkDebugUtilsMessengerEXT` in addition to a
     *          `VkInstance`.
     * @param application_name The application's name. This is passed to Vulkan but probably does nothing.
     * @param application_version The application's version as if generated by `VK_MAKE_API_VERSION`. This is
     *                            passed to Vulkan but probably does nothing.
     * @param engine_name The engine's name. This is passed to Vulkan but probably does nothing.
     * @param engine_version The engine's version as if generated by `VK_MAKE_API_VERSION`. This is passed to Vulkan
     *                       but probably does nothing.
     * @param requested_layers A set of requested Vulkan layers to initialize along with the `VkInstance`.
     * @param required_extensions A set of required Vulkan instance-level extensions. If any of these extensions are
     *                            unsupported then an error will be thrown.
     * @param requested_extensions A set of requested Vulkan instance-level extensions. Any unsupported extension in
     *                             this set will simply be ignored.
     */
    debug_graphics_context(const std::string& application_name, const u32 application_version,
                           const std::string& engine_name, const u32 engine_version,
                           const std::unordered_set<std::string>& requested_layers,
                           const std::unordered_set<std::string>& required_extensions,
                           const std::unordered_set<std::string>& requested_extensions);

    /// @cond
    debug_graphics_context(const debug_graphics_context& other) = delete;
    debug_graphics_context(debug_graphics_context&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `debug_graphics_context`.
     */
    ~debug_graphics_context() noexcept;

    /// @cond
    debug_graphics_context& operator=(const debug_graphics_context& rhs) = delete;
    debug_graphics_context& operator=(debug_graphics_context&& rhs) = delete;
    /// @endcond
  };

}

#endif
