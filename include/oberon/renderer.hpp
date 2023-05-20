#ifndef OBERON_RENDERER_HPP
#define OBERON_RENDERER_HPP

#include <unordered_set>

#include "memory.hpp"
#include "rects.hpp"

#include "concepts/has_internal_implementation.hpp"

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(renderer_impl);

}

namespace oberon {

  class graphics_device;
  class window;
  class frame;

  /**
   * @class renderer
   * @brief
   *
   */
  class renderer final {
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::renderer_impl);
  public:
    using implementation_type = internal::base::renderer_impl;

    /**
     * @brief Create a renderer.
     * @param device The `graphics_device` to derive the `renderer` from.
     * @param resolution The resolution of rendered images produced the `renderer`.
     */
    renderer(graphics_device& device, const extent_2d& resolution, const u32 samples);

    /**
     * @brief Create a renderer matched to a window.
     * @details This constructs a `renderer` with settings that match the input `window`. No dependency on the input
     *          `window` is created. That is to say, you may still call `begin_frame()` on another `window`.
     * @param device The `graphics_device` to derive the `renderer` from.
     * @param win The `window` to match.
     */
    renderer(graphics_device& device, window& win, const u32 samples);

    renderer(const renderer& other) = delete;
    renderer(renderer&& other) = delete;

    ~renderer() noexcept = default;

    renderer& operator=(const renderer& rhs) = delete;
    renderer& operator=(renderer&& rhs) = delete;

    /**
     * @brief Retrieve the internal `renderer` implementation.
     *
     * @return A reference to the `renderer`'s implementation object.
     */
    implementation_type& implementation();

    /**
     * @brief Begin rendering a frame targeting the input `window`.
     * @details This begins rendering on the next available `renderer` frame. The frame's attachments will be
     *          cleared and the frame will be marked as being in use. If all of the `renderer`'s frames are busy when
     *          this is called then the application will block until a frame becomes available. An image will also be
     *          acquired from the input `window`.
     * @param win The `window` to acquire an image from.
     * @return The frame to render to.
     */
    frame begin_frame(window& win);


    /**
     * @brief Finish rendering a frame and present it to the frame's window.
     * @details This ends rendering on the input `frame`. The resulting color image will be copied to the acquired
     *          `window` image and submitted for presentation. If the window and renderer settings do not match then
     *          the image will instead be blit to the `window` image.
     * @param fr The completed frame to submit.
     */
    void end_frame(frame&& fr);

  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, renderer);

}

#endif
