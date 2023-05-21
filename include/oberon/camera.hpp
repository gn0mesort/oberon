/**
 * @file camera.hpp
 * @brief 3D camera object.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_CAMERA_HPP
#define OBERON_CAMERA_HPP

#include "memory.hpp"
#include "glm.hpp"

namespace oberon {

  /**
   * @class camera
   * @brief An object representing a view into 3D space.
   */
  class camera final {
  private:
    glm::mat4 m_view{ glm::identity<glm::mat4>() };
    glm::mat4 m_projection{ };
  public:
    /**
     * @brief Create a `camera` with the specified projection matrix.
     * @param projection The projection matrix for the newly created camera.
     */
    camera(const glm::mat4& projection);

    /**
     * @brief Create a `camera` by copying another `camera`.
     * @param other The `camera` to copy.
     */
    camera(const camera& other) = default;

    /**
     * @brief Create a `camera` by moving another `camera`.
     * @param other The `camera` to move.
     */
    camera(camera&& other) = default;

    /**
     * @brief Destroy a `camera`.
     */
    ~camera() noexcept = default;

    /**
     * @brief Assign the `camera`'s state so that it matches another `camera`.
     * @param rhs The `camera` to copy the state of.
     * @return A reference to the `camera`.
     */
    camera& operator=(const camera& rhs) = default;

    /**
     * @brief Assign the `camera`'s state by moving the state of another `camera`.
     * @param rhs The `camera` to move the state of.
     * @return A reference to the `camera`.
     */
    camera& operator=(camera&& rhs) = default;

    /**
     * @brief Reposition the `camera` to `position` and orient it so that it is pointing at `target`.
     * @param position The 3D position to move the `camera` to.
     * @param target The 3D position to point the `camera` towards.
     */
    void look_at(const glm::vec3& position, const glm::vec3& target);
  };

}

#endif
