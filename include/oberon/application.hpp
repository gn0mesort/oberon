/**
 * @file application.hpp
 * @brief The liboberon application object.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_APPLICATION_HPP
#define OBERON_APPLICATION_HPP

#include <functional>

#include "types.hpp"
#include "memory.hpp"

namespace oberon {

  class system;

  /**
   * @class application
   * @brief An object representing an application of the Oberon library.
   * @details `application`s represent individual initializations of the Oberon library. When an `application` is run,
   *           a `system` object is initialized according to the platform configuration. Once initialized, the
   *           provided `entry_point` is executed.
   */
  class application final {
  public:
    /**
     * @brief The entry point function for `application`s.
     */
    using entry_point = int(const i32, const basic_sequence<csequence>, system&);

    /**
     * @brief Run the `application`.
     * @param fn The `entry_point` to execute.
     * @param argc The length of the `argv` sequence. This must be greater than or equal to 1.
     * @param argv The `application` argument sequence. This must contain a sequence of null terminated strings which
     *             is itself null terminated. That is to say, `argv[argc]` must equal `nullptr`. Similarly, `argv[0]`
     *             must contain the name of the application as a null terminated string.
     * @return A status code indicating the exit condition of the `application`. Successful executions must return a
     *         value such that the least significant byte is 0. Failures must return a value such that the the least
     *         significant byte is greater than 0.
     */
    int run(const std::function<entry_point>& fn, const i32 argc, const basic_sequence<csequence> argv);

    /**
     * @brief Run the `application` with only the specified device initialized.
     * @details In general, Oberon will initialize every compatible physical device on the system. However, this
     *          might be undesirable in certain situations. Instead, `run_device_exclusive()` will only initialize the
     *          device specified by `uuid`.
     * @param uuid The UUID representing the device to initialize. This must be a valid UUID string.
     *             See @link{https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap5.html#VkPhysicalDeviceVulkan11Properties}
     *             for more information on Vulkan device UUIDs.
     * @param fn The `entry_point` to execute.
     * @param argc The length of the `argv` sequence. This must be greater than or equal to 1.
     * @param argv The `application` argument sequence. This must contain a sequence of null terminated strings which
     *             is itself null terminated. That is to say, `argv[argc]` must equal `nullptr`. Similarly, `argv[0]`
     *             must contain the name of the application as a null terminated string.
     * @return A status code indicating the exit condition of the `application`. Successful executions must return a
     *         value such that the least significant byte is 0. Failures must return a value such that the the least
     *         significant byte is greater than 0.
     */
    int run_device_exclusive(const std::string& uuid, const std::function<entry_point>& fn, const i32 argc,
                             const basic_sequence<csequence> argv);
  };

}

#endif
