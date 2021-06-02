#ifndef OBERON_DETAIL_CONTEXT_IMPL_HPP
#define OBERON_DETAIL_CONTEXT_IMPL_HPP

#include "../context.hpp"

#include "object_impl.hpp"

#include "graphics.hpp"
#include "vulkan_function_table.hpp"

namespace oberon {
namespace detail {
  struct context_impl : public object_impl { 
    ptr<xcb_connection_t> x_connection{ };
    ptr<xcb_screen_t> x_screen{ };

    PFN_vkGetInstanceProcAddr loader{ };
    ptr<vulkan_function_table> vkft{ };
    VkInstance instance{ };
    VkPhysicalDevice physical_device{ };
    u32 graphics_transfer_queue_family{ };
    u32 presentation_queue_family{ };
    VkDevice device{ };
    VkQueue graphics_transfer_queue{ };
    VkQueue presentation_queue{ };

    inline virtual ~context_impl() noexcept = 0;
  };

  context_impl::~context_impl() noexcept { }

  void context_initialize_x(context_impl& ctx, const cstring displayname);
  void context_load_vulkan_library(context_impl& ctx);
  void context_load_global_pfns(context_impl& ctx);
  void context_initialize_vulkan_instance(
    context_impl& ctx,
    const cstring application_name,
    const u32 application_version,
    const readonly_ptr<cstring> layers,
    const u32 layer_count,
    const readonly_ptr<cstring> extensions,
    const u32 extension_count,
    const readonly_ptr<void> next
  );
  void context_load_instance_pfns(context_impl& ctx);
  void context_initialize_vulkan_device(
    context_impl& ctx,
    const readonly_ptr<cstring> extensions,
    const u32 extension_count,
    const readonly_ptr<VkPhysicalDeviceFeatures> features,
    const readonly_ptr<VkDeviceQueueCreateInfo> queue_infos,
    const u32 queue_info_count,
    const readonly_ptr<void> next
  );
  void context_load_device_pfns(context_impl& ctx);

#define OBERON_GLOBAL_FN(ctx, name) \
  (context_load_global_fn<PFN_##name>((ctx), (#name)))

  template <typename FunctionPointer>
  FunctionPointer context_load_global_fn(const context_impl& ctx, const cstring name) {
    auto pfn = reinterpret_cast<FunctionPointer>(ctx.loader(nullptr, name));
    if (!pfn)
    {
      throw fatal_error{ "Failed to load global function: \"" + std::string{ name } + "\"." };
    }
    return pfn;
  }

#define OBERON_INSTANCE_FN(ctx, name) \
  (context_load_instance_fn<PFN_##name>((ctx), (#name)))

  template <typename FunctionPointer>
  FunctionPointer context_load_instance_fn(const context_impl& ctx, const cstring name) {
    if (!ctx.instance)
    {
      throw fatal_error{ "Cannot load function because the Vulkan instance is not valid." };
    }
    auto pfn = reinterpret_cast<FunctionPointer>(ctx.loader(ctx.instance, name));
    if (!pfn)
    {
      throw fatal_error{ "Failed to load instance function: \"" + std::string{ name } + "\"." }; 
    }
    return pfn;
  }

#define OBERON_DEVICE_FN(ctx, name) \
  (context_load_device_fn<PFN_##name>((ctx), (#name)))

  template <typename FunctionPointer>
  FunctionPointer context_load_device_fn(const context_impl& ctx, const cstring name) {
    if (!ctx.device)
    {
      throw fatal_error{ "Cannot load function because the Vulkan device is not valid." };
    }
    auto pfn = reinterpret_cast<FunctionPointer>(ctx.vkft->vkGetDeviceProcAddr(ctx.device, name));
    if (!pfn)
    {
      throw fatal_error{ "Failed to load device function: \"" + std::string{ name } + "\"." };
    }
    return pfn;
  }

  void context_deinitialize_vulkan_device(context_impl& ctx);
  void context_deinitialize_vulkan_instance(context_impl& ctx);
  void context_deinitialize_x(context_impl& ctx);
}
}

#endif
