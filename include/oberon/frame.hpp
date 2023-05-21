/**
 * @file frame.hpp
 * @brief Render frame object.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_FRAME_HPP
#define OBERON_FRAME_HPP

#include "memory.hpp"
#include "glm.hpp"

#include "concepts/has_internal_implementation.hpp"

namespace oberon::internal::base {

  class frame_impl;
  struct frame_info;

}

namespace oberon {

  class camera;
  class mesh;

  /**
   * @class frame
   * @brief An object representing a single renderable frame.
   * @details `frame`s can neither be created nor destroyed by the application.
   *          Instead a `frame` always belongs to a `renderer`. `renderer`s are
   *          responsible for controlling the lifetime of `frame`s. Destroying
   *          a `frame` via its destructor will only recycle its resources
   *          back to the parent `renderer`.
   */
  class frame final {
  private:
    ptr<internal::base::frame_impl> m_impl{ };
  public:
    using implementation_type = internal::base::frame_impl;

    /// @cond
    frame(internal::base::frame_impl& impl);
    frame(const frame& other) = delete;
    /// @endcond

    /**
     * @brief Create a new `frame` by moving the state of an existing `frame`.
     * @param other The `frame` to move the state of.
     */
    frame(frame&& other) = default;

    /**
     * @brief Destroy a `frame`.
     * @details Destroying a `frame` does not release any resources. Instead,
     *          the `frame`'s resources are returned to the parent `renderer`.
     */
    ~frame() noexcept = default;

    /// @cond
    frame& operator=(const frame& rhs) = delete;
    /// @endcond
    /**
     * @brief Assign a `frame` by moving the state of another `frame`.
     * @param rhs The `frame` to move the state of.
     * @return A reference to the `frame`.
     */
    frame& operator=(frame&& rhs) = default;

    /**
     * @brief Retrieve the `frame`'s implementation.
     * @return A reference to the `frame`'s implementation object.
     */
    implementation_type& implementation();

    /**
     * @brief Determine whether a `frame` is retired or not.
     * @details Whenever a `frame` is moved the old location of the `frame` enters the retired state. When in this
     *          state the frame cannot for any operation other than move assignment.
     * @return True if the `frame` has been retired. False in all other cases.
     */
    bool is_retired() const;
    /**
     * @brief Draw a test pattern image.
     */
    void draw_test_image();

    /**
     * @brief Draw a `mesh` using the perspective of the specified `camera`.
     * @param c The `camera` to use when drawing the `mesh`.
     * @param m The `mesh` to draw.
     */
    void draw(camera& c, mesh& m);
  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, frame);

}

#endif
