#ifndef OBERON_INTERNAL_LINUX_X11_RENDER_WINDOW_HPP
#define OBERON_INTERNAL_LINUX_X11_RENDER_WINDOW_HPP

#include <array>
#include <vector>

#include <nng/nng.h>

#include "../../../types.hpp"

#include "../../base/vulkan.hpp"
#include "../../base/render_window_impl.hpp"

#include "xcb.hpp"
#include "keys.hpp"

namespace oberon {

  class graphics_device;

}

namespace oberon::internal::linux::x11 {


namespace query_visibility_flag_bits {

  OBERON_DEFINE_ZERO_BIT(none);
  OBERON_DEFINE_BIT(mapped, 0);
  OBERON_DEFINE_BIT(iconic, 1);
  OBERON_DEFINE_BIT(hidden, 2);

}

  class render_window_impl final : public base::render_window_impl {
  private:
    struct key_state final {
      bool pressed{ };
      bool echoing{ };
    };

    // I know this is just 1 bool.
    // std::vector<bool> has weird behavior.
    struct mouse_button_state final {
      bool pressed{ };
    };

    static constexpr const usize FRAME_COUNT{ 2 };
    static constexpr const usize SEMAPHORES_PER_FRAME{ 2 };
    static constexpr const usize IMAGE_ACQUIRED_SEMAPHORE_INDEX{ 0 };
    static constexpr const usize RENDER_FINISHED_SEMAPHORE_INDEX{ 1 };
    static constexpr const usize COMMAND_BUFFERS_PER_FRAME{ 2 };
    static constexpr const usize TRANSFER_COMMAND_BUFFER_INDEX{ 0 };
    static constexpr const usize GRAPHICS_COMMAND_BUFFER_INDEX{ 1 };

    ptr<graphics_device> m_parent_device{ };
    xcb_window_t m_window{ };
    ptr<xkb_keymap> m_keyboard_map{ };
    ptr<xkb_state> m_keyboard_state{ };
    std::array<xcb_keycode_t, MAX_KEY> m_to_keycode{ };
    // This is a little weird. The number of xcb_keycode_t -> oberon::key mappings is the number of unique
    // xcb_keycode_t values that can be represented (i.e., 256 and not 255). The reason this isn't MAX_KEY is that
    // MAX_KEY is the number of valid usize -> xcb_keycode_t mappings (i.e., a number closer to 128).
    std::array<oberon::key, std::numeric_limits<xcb_keycode_t>::max() + 1> m_to_external_key{ };
    std::array<xkb_mod_index_t, MAX_MODIFIER_KEY> m_to_modifier_index{ };
    std::array<key_state, MAX_KEY> m_key_states{ };
    std::vector<mouse_button_state> m_mouse_button_states{ };

    nng_socket m_socket{ };
    nng_dialer m_dialer{ };

    VkSurfaceKHR m_surface{ };
    VkExtent2D m_swapchain_extent{ };
    VkSurfaceFormatKHR m_swapchain_surface_format{ };
    VkSwapchainKHR m_swapchain{ };
    VkFormat m_depth_stencil_format{ };
    std::vector<VkImage> m_swapchain_images{ };
    std::vector<VkImageView> m_swapchain_image_views{ };
    std::array<VkFence, FRAME_COUNT> m_frame_fences{ };
    std::array<VkSemaphore, SEMAPHORES_PER_FRAME * FRAME_COUNT> m_frame_semaphores{ };
    std::array<VkCommandPool, FRAME_COUNT> m_frame_command_pools{ };
    std::array<VkCommandBuffer, COMMAND_BUFFERS_PER_FRAME * FRAME_COUNT> m_frame_command_buffers{ };
    std::array<VkImage, FRAME_COUNT> m_frame_depth_stencil_images{ };
    std::array<VkImageView, FRAME_COUNT> m_frame_depth_image_views{ };
    std::array<VkImageView, FRAME_COUNT> m_frame_stencil_image_views{ };

    void send_client_message(const xcb_window_t destination, const xcb_generic_event_t& message);
    void change_size_hints(const xcb_size_hints_t& hints);
    void initialize_keyboard();
    void deinitialize_keyboard();
    bitmask query_visibility() const;
    void change_ewmh_states(const ewmh_state_action action, const xcb_atom_t first, const xcb_atom_t second);
    void change_compositor_mode(const compositor_mode mode);
    void initialize_swapchain(const VkSwapchainKHR old);
    void deinitialize_swapchain();
  public:
    render_window_impl(graphics_device& device, const std::string& title, const rect_2d& bounds);
    render_window_impl(const render_window_impl& other) = delete;
    render_window_impl(render_window_impl&& other) = delete;

    ~render_window_impl() noexcept;

    render_window_impl& operator=(const render_window_impl& other) = delete;
    render_window_impl& operator=(render_window_impl&& other) = delete;

    u32 id() const override;
    void show() override;
    void hide() override;
    bool is_shown() const override;
    bool is_minimized() const override;
    void change_display_style(const display_style style) override;
    display_style current_display_style() const override;
    rect_2d current_drawable_rect() const override;
    rect_2d current_rect() const override;
    void change_title(const std::string& title) override;
    std::string title() const override;
    void resize(const extent_2d& extent) override;
    void move_to(const offset_2d& offset) override;
    event poll_events() override;
    oberon::key translate_keycode(const u32 code) const override;
    oberon::mouse_button translate_mouse_buttoncode(const u32 code) const override;
    bool is_modifier_pressed(const oberon::modifier_key modifier) const override;
    bool is_key_pressed(const oberon::key k) const override;
    bool is_key_echoing(const oberon::key k) const override;
    bool is_mouse_button_pressed(const oberon::mouse_button mb) const override;
  };

}

#endif
