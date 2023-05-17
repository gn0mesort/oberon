#ifndef OBERON_INTERNAL_BASE_RENDER_WINDOW_HPP
#define OBERON_INTERNAL_BASE_RENDER_WINDOW_HPP

#include <unordered_set>
#include <string>

#include "../../events.hpp"
#include "../../keys.hpp"
#include "../../mouse.hpp"
#include "../../render_window.hpp"

#include "vulkan.hpp"

namespace oberon {

  class camera;
  class mesh;

}

namespace oberon::internal::base {

  class render_window_impl {
  protected:
    render_window_impl() = default;
  public:
    render_window_impl(const render_window_impl& other) = delete;
    render_window_impl(render_window_impl&& other) = delete;

    virtual ~render_window_impl() noexcept = default;

    render_window_impl& operator=(const render_window_impl& other) = delete;
    render_window_impl& operator=(render_window_impl&& other) = delete;

    virtual u32 id() const = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual bool is_shown() const = 0;
    virtual bool is_minimized() const = 0;
    virtual void change_display_style(const display_style style) = 0;
    virtual display_style current_display_style() const = 0;
    virtual rect_2d current_drawable_rect() const = 0;
    virtual rect_2d current_rect() const = 0;
    virtual void change_title(const std::string& title) = 0;
    virtual std::string title() const = 0;
    virtual void resize(const extent_2d& extent) = 0;
    virtual void move_to(const offset_2d& offset) = 0;
    virtual event poll_events() = 0;
    virtual bool is_key_pressed(const key k) const = 0;
    virtual bool is_key_echoing(const key k) const = 0;
    virtual bool is_mouse_button_pressed(const mouse_button mb) const = 0;
    virtual key translate_keycode(const u32 code) const = 0;
    virtual mouse_button translate_mouse_buttoncode(const u32 code) const = 0;
    virtual bool is_modifier_pressed(const modifier_key modifier) const = 0;
    virtual void draw_test_image() = 0;
    virtual void swap_buffers() = 0;
    virtual const std::unordered_set<presentation_mode>& available_presentation_modes() const = 0;
    virtual void request_presentation_mode(const presentation_mode mode) = 0;
    virtual presentation_mode current_presentation_mode() const = 0;
    virtual void copy_buffer(VkBuffer from, VkBuffer to, const u32 size) = 0;
    virtual void insert_memory_barrier(const VkMemoryBarrier& barrier, const VkPipelineStageFlags src,
                                       const VkPipelineStageFlags dest) = 0;
    virtual void change_active_camera(camera& cam) = 0;
    virtual void draw(mesh& m) = 0;
    virtual void clear_active_camera() = 0;
  };

}

#endif
