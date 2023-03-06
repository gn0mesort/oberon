/**
 * @file application.cpp
 * @brief Application class implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/application.hpp"

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <vector>
#include <string>

#include "oberon/debug.hpp"
#include "oberon/errors.hpp"
#include "oberon/platform.hpp"

// Platform specific includes should be selected here.
// The proper set of enabled macros should be selected during build configuration by Meson.
// For example, on a Linux system Meson should define MESON_SYSTEM_LINUX as 1 and define MESON_SYSTEM to the string
// "linux". Additional platforms should follow the same scheme (e.g., define MESON_SYSTEM_WINDOWS as 1 and set
// MESON_SYSTEM to an appropriate string).
#ifdef MESON_SYSTEM_LINUX
#include <getopt.h>
#include <libgen.h>

#include "oberon/linux/system.hpp"
#include "oberon/linux/input.hpp"
#include "oberon/linux/window.hpp"
#include "oberon/linux/platform.hpp"
#endif

namespace oberon {

  int application::run(const std::function<entry_point>& fn, const int argc, const ptr<csequence> argv) {
    OBERON_CHECK(argv[argc] == nullptr);
    // The result *as-if* returned from a standard main procedure.
    // If an error occurs the least significant byte of the result must not be 0.
    // If the entry point function exits normally (e.g., via user request) the least significant byte must be 0.
    // For example, a valid error result might be 0xff'ff'ff'01 (assuming int = i32).
    // Conversly, a valid success result might be 0xff'ff'ff'00 (assuming int = i32).
    auto result = int{ 0 };
    try
    {
// Platform selection should occur here.
// Based on compilation settings (and the underlying compilation platform), Meson should select the correct set of
// macros for the target platform.
// The order of subsystem creation should be the reverse of the generic tear down.
#ifdef MESON_SYSTEM_LINUX
      // Parse arguments to find the ICCCM instance name. This is used by WM_CLASS.
      // see https://www.x.org/releases/current/doc/xorg-docs/icccm/icccm.html#WM_CLASS_Property
      auto name = std::string{ };
      {
        enum { NAME_FOUND };
        auto longopts = std::array<::option, 2>{  ::option{ "name", required_argument, nullptr, NAME_FOUND },
                                                  ::option{ nullptr, 0, nullptr, 0 } };
        auto option_index = int{ };
        for (auto opt = -1; (opt = getopt_long_only(argc, argv, "", longopts.data(), &option_index)) != -1;)
        {
          switch (opt)
          {
          case NAME_FOUND:
            name = optarg;
            break;
          default:
            break;
          }
        }
        if (name.empty())
        {
          auto env_name = std::getenv("RESOURCE_NAME");
          if (env_name && std::strlen(env_name))
          {
            name = env_name;
          }
          else
          {
            auto path = strdup(argv[0]);
            name = basename(path);
            std::free(path);
          }
        }
      }
      // Get desired Vulkan layer list, if any, and split it.
      auto desired_vk_layers = std::vector<std::string>{ };
      {
        auto env_vk_layers = std::getenv("OBERON_VK_LAYERS");
        if (env_vk_layers)
        {
          auto vk_layer_list = strdup(env_vk_layers);
          auto vk_layer_list_ptr = &vk_layer_list;
          auto res = cstring{ };
          while ((res = strsep(vk_layer_list_ptr, ",")))
          {
            if (std::strcmp(res, ""))
            {
              desired_vk_layers.emplace_back(res);
            }
          }
          std::free(vk_layer_list);
        }
  #ifndef NDEBUG
        if (desired_vk_layers.empty())
        {
          desired_vk_layers.emplace_back("VK_LAYER_KHRONOS_validation");
        }
  #endif
      }
      auto platform_system = new linux::system{ name, "oberon", desired_vk_layers };
      auto platform_input = new linux::input{ *platform_system };
      auto platform_window = new linux::window{ *platform_system };
      auto plt = linux::platform{ *platform_system, *platform_input, *platform_window };
#endif
      result = fn(argc, argv, plt);
      // Tear down platform specific objects.
      delete platform_window;
      delete platform_input;
      delete platform_system;
    }
    // oberon::errors are caught here.
    // These carry unique error codes, std::source_locations, and the standard "what()" message.
    catch (const error& err)
    {
      std::cerr << err.type() << " (" << err.location().file_name() << ":" << err.location().line() << ":"
                << err.location().column() << "): " << err.message() << std::endl;
      result = err.result();
    }
    // Standard exceptions carry less information than equivalent oberon types.
    // As a result, when a standard exception is caught the result is always 1. This is likely to be equivalent to
    // EXIT_FAILURE.
    catch (const std::exception& err)
    {
      std::cerr << "std::exception (unknown): " << err.what() << std::endl;
      result = 1;
    }
    return result;
  }

}
