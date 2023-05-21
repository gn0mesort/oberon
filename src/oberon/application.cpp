/**
 * @file application.cpp
 * @brief Application object implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/application.hpp"

#include <cstring>
#include <cstdlib>

#include <iostream>
#include <string>
#include <array>
#include <unordered_set>

#include "oberon/memory.hpp"
#include "oberon/errors.hpp"
#include "oberon/system.hpp"

#include "configuration.hpp"

#include "oberon/internal/base/vulkan.hpp"
#include "oberon/internal/base/system_impl.hpp"
#include "oberon/internal/base/graphics_context.hpp"

#ifdef CONFIGURATION_OPERATING_SYSTEM_LINUX
  #include <getopt.h>
  #include <libgen.h>

  #include "oberon/internal/linux/utility.hpp"

  #ifdef CONFIGURATION_WINDOW_SYSTEM_X11
    #include "oberon/internal/linux/x11/xcb.hpp"
    #include "oberon/internal/linux/x11/system_impl.hpp"
    #include "oberon/internal/linux/x11/wsi_context.hpp"
  #endif
#endif

namespace oberon {

  inline ptr<system> initialize_system(const readonly_ptr<std::string> uuid, const i32 argc,
                                       const basic_sequence<csequence> argv) {
    // Prepare for Linux system initialization.
#ifdef CONFIGURATION_OPERATING_SYSTEM_LINUX
    // Parse arguments to find the ICCCM instance name. This is used by WM_CLASS.
    // see https://www.x.org/releases/current/doc/xorg-docs/icccm/icccm.html#WM_CLASS_Property
    auto instance_name = std::string{ };
    {
      constexpr auto NAME_FOUND = int{ 1 };
      const auto longopts = std::array<::option, 2>{ ::option{ "name", required_argument, nullptr, NAME_FOUND },
                                                     ::option{ nullptr, 0, nullptr, 0 } };
      auto option_index = int{ };
      for (auto opt = -1; (opt = getopt_long_only(argc, argv, "", longopts.data(), &option_index)) != -1;)
      {
        switch (opt)
        {
        case NAME_FOUND:
          instance_name = optarg;
        default:
          break;
        }
      }
      if (instance_name.empty())
      {
        auto env_name = std::getenv("RESOURCE_NAME");
        if (env_name && std::strlen(env_name))
        {
          instance_name = env_name;
        }
        else
        {
          // Basename *may* modify its parameter.
          auto path = strdup(argv[0]);
          instance_name = basename(path);
          std::free(path);
        }
      }
    }
    // Parse Vulkan layer names provided by the environment.
    auto requested_layers = std::unordered_set<std::string>{ };
    {
      const auto env_layer_list = std::getenv("OBERON_VK_LAYERS");
      if (env_layer_list)
      {
        // strsep modifies its first parameter.
        auto layer_list = strdup(env_layer_list);
        // strsep treats stringp as an in/out reference so a copy of layer_list is required. Otherwise the
        // duplicated string's pointer would be lost.
        auto itr = layer_list;
        auto res = cstring{ };
        while ((res = strsep(&itr, ",")))
        {
          if (std::strcmp(res, ""))
          {
            requested_layers.insert(res);
          }
        }
        std::free(layer_list);
      }
    }
#endif
    // Platform selection should occur here.
    // Based on compilation settings (and the underlying compilation platform), Meson should select the correct set of
    // macros for the target platform.
    //
    // Linux, additionally, requires a window system to be selected for a complete system definition to be built.
#if defined(CONFIGURATION_OPERATING_SYSTEM_LINUX) && defined(CONFIGURATION_WINDOW_SYSTEM_X11)
    auto x11_ctx = new internal::linux::x11::wsi_context{ instance_name };
    const auto engine_version = VK_MAKE_API_VERSION(0, configuration::version_major(), configuration::version_minor(),
                                                    configuration::version_patch());
    // VK_KHR_surface and VK_KHR_xcb_surface are required to be supported by the instance.
    const auto required_extensions = std::unordered_set<std::string>{ VK_KHR_SURFACE_EXTENSION_NAME,
                                                                      VK_KHR_XCB_SURFACE_EXTENSION_NAME };
  // In debug builds, the debug_graphics_context should be used rather than the default graphics_context.
  // This will create the underlying Vulkan instance with validation features enabled (if available) and
  // prepare a debug callback messenger.
  #ifndef NDEBUG
    auto vk_ctx = new internal::base::debug_graphics_context{ x11_ctx->instance_name(), 0, x11_ctx->application_name(),
                                                              engine_version, requested_layers, required_extensions,
                                                              { } };
  #else
    auto vk_ctx = new internal::base::graphics_context{ x11_ctx->instance_name(), 0, x11_ctx->application_name(),
                                                        engine_version, requested_layers, required_extensions, { } };
  #endif
    // Technically this is an array.
    uuid_t device_uuid;
    if (uuid)
    {
      OBERON_CHECK_ERROR_MSG(!uuid_parse(uuid->data(), device_uuid), 1, "Failed to parse device UUID. The UUID "
                             "\"%s\" was invalid.", uuid->data());
      internal::linux::x11::system_impl::initialize_only(device_uuid);
    }
    auto sys_impl = new internal::linux::x11::system_impl{ std::move(x11_ctx), std::move(vk_ctx) };
#else
  #error There must be at least one valid implementation available to the application.
#endif
    return new system{ std::move(sys_impl) };
  }

  inline int error_handler(const readonly_ptr<std::string> uuid, const std::function<application::entry_point>& fn,
                                 const i32 argc, const basic_sequence<csequence> argv) {
    // The result *as-if* returned from a standard main procedure.
    // If an error occurs the least significant byte of the result must not be 0.
    // If the entry point function exits normally (e.g., via user request) the least significant byte must be 0.
    // For example, a valid error result might be 0xff'ff'ff'01 (assuming int = i32).
    // Conversly, a valid success result might be 0xff'ff'ff'00 (assuming int = i32).
    auto result = i32{ };
    try
    {
      auto sys = initialize_system(uuid, argc, argv);
      std::cerr << configuration::build_string() << std::endl;
      result = fn(argc, argv, *sys);
      delete sys;
    }
    // oberon::errors are caught here.
    // These carry unique error codes, std::source_locations, and the standard "what()" message.
    catch (const error& err)
    {
      std::cerr << err.type() << " (" << err.location().file_name() << ":" << err.location().line() << ":"
                << err.location().column() << "): " << err.message() << std::endl;
      std::cerr << "Status: " << err.result() << std::endl;
      result = err.result();
    }
    // Standard exceptions carry less information than equivalent oberon types.
    // As a result, when a standard exception is caught the result is always 1. This is likely to be equivalent to
    // EXIT_FAILURE.
    catch (const std::exception& err)
    {
      std::cerr << "std::exception (unknown): " << err.what() << std::endl;
      std::cerr << "Status: 1" << std::endl;
      result = 1;
    }
    return result;
  }

  int application::run(const std::function<entry_point>& fn, const i32 argc, const basic_sequence<csequence> argv) {
    return error_handler(nullptr, fn, argc, argv);
  }

  int application::run_device_exclusive(const std::string& uuid, const std::function<entry_point>& fn, const i32 argc,
                                        const basic_sequence<csequence> argv) {
    return error_handler(&uuid, fn, argc, argv);
  }

}
