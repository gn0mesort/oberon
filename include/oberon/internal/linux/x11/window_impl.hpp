#ifndef OBERON_INTERNAL_LINUX_X11_WINDOW_IMPL_HPP
#define OBERON_INTERNAL_LINUX_X11_WINDOW_IMPL_HPP

#include <array>
#include <vector>

#include <nng/nng.h>

#include "../../../types.hpp"

#include "../../base/vulkan.hpp"
#include "../../base/graphics_device_impl.hpp"
#include "../../base/window_impl.hpp"

#include "xcb.hpp"
#include "keys.hpp"

namespace oberon {

  class graphics_device;

}

namespace oberon::internal::linux::x11 {

  class graphics_device_impl;

namespace query_visibility_flag_bits {

  OBERON_DEFINE_ZERO_BIT(none);
  OBERON_DEFINE_BIT(mapped, 0);
  OBERON_DEFINE_BIT(iconic, 1);
  OBERON_DEFINE_BIT(hidden, 2);

}

namespace renderer_status_flag_bits {

  OBERON_DEFINE_ZERO_BIT(none);
  OBERON_DEFINE_BIT(dirty, 0);

}

  class window_impl final : public base::window_impl {
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

    ptr<graphics_device_impl> m_parent{ };
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
    std::unordered_set<enum presentation_mode> m_presentation_modes{ };
    VkPresentModeKHR m_swapchain_present_mode{ VK_PRESENT_MODE_FIFO_KHR };
    VkExtent2D m_swapchain_extent{ };
    VkSurfaceFormatKHR m_swapchain_surface_format{ };
    VkSwapchainKHR m_swapchain{ };
    std::vector<VkImage> m_swapchain_images{ };
    std::vector<VkImageView> m_swapchain_image_views{ };
    bitmask m_status{ };


    void send_client_message(const xcb_window_t destination, const xcb_generic_event_t& message);
    void change_size_hints(const xcb_size_hints_t& hints);
    void initialize_keyboard();
    void deinitialize_keyboard();
    bitmask query_visibility() const;
    void change_ewmh_states(const ewmh_state_action action, const xcb_atom_t first, const xcb_atom_t second);
    void change_compositor_mode(const compositor_mode mode);
    void initialize_swapchain(const VkSwapchainKHR old);
  public:
    window_impl(graphics_device& device, const std::string& title, const rect_2d& bounds);
    window_impl(const window_impl& other) = delete;
    window_impl(window_impl&& other) = delete;

    ~window_impl() noexcept;

    window_impl& operator=(const window_impl& other) = delete;
    window_impl& operator=(window_impl&& other) = delete;

    u32 acquire_next_image(const VkSemaphore acquired) override;
    const std::vector<VkImage>& swapchain_images() override;
    VkFormat surface_format() const override;
    VkExtent2D swapchain_extent() const override;
    void present_image(const u32 index, const VkSemaphore ready_to_present) override;
    u32 id() const override;
    void show() override;
    void hide() override;
    bool is_shown() const override;
    bool is_minimized() const override;
    void display_style(const enum display_style style) override;
    enum display_style display_style() const override;
    rect_2d drawable_rect() const override;
    rect_2d screen_rect() const override;
    void title(const std::string& title) override;
    std::string title() const override;
    void resize(const extent_2d& extent) override;
    void move_to(const offset_2d& offset) override;
    event poll_events() override;
    oberon::key translate_keycode(const u32 code) const override;
    oberon::mouse_button translate_mouse_buttoncode(const u32 code) const override;
    bool is_modifier_key_active(const oberon::modifier_key modifier) const override;
    bool is_key_pressed(const oberon::key k) const override;
    bool is_key_echoing(const oberon::key k) const override;
    bool is_mouse_button_pressed(const oberon::mouse_button mb) const override;
    const std::unordered_set<enum presentation_mode>& available_presentation_modes() const override;
    void presentation_mode(const enum presentation_mode mode) override;
    enum presentation_mode presentation_mode() const override;
  };

}

#endif
