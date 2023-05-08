#ifndef OBERON_INTERNAL_LINUX_X11_WSI_CONTEXT_HPP
#define OBERON_INTERNAL_LINUX_X11_WSI_CONTEXT_HPP

#include <array>

#include <pthread.h>

#include "../../../types.hpp"
#include "../../../memory.hpp"


#include "xcb.hpp"
#include "atoms.hpp"

namespace oberon::internal::linux::x11 {


  class wsi_context final {
  private:
    std::string m_instance_name{ };
    std::string m_application_name{ "oberon" };
    ptr<xcb_connection_t> m_connection{ };
    readonly_ptr<xcb_setup_t> m_setup{ };
    ptr<xcb_screen_t> m_default_screen{ };
    u8 m_xi_major_opcode{ };
    u8 m_xkb_first_event{ };
    xcb_input_device_id_t m_xi_master_keyboard_id{ };
    xcb_input_device_id_t m_xi_master_pointer_id{ };
    ptr<xkb_context> m_xkb_context{ };
    std::array<xcb_atom_t, MAX_ATOM> m_atoms{ };
    pthread_t m_wsi_pub_worker{ };
    xcb_window_t m_client_leader{ };

    void send_client_message(const xcb_window_t window, const xcb_client_message_event_t& message);
  public:
    wsi_context();
    explicit wsi_context(const std::string& instance);
    wsi_context(const cstring display, const std::string& instance);
    wsi_context(const wsi_context& other) = delete;
    wsi_context(wsi_context&& other) = delete;

    ~wsi_context() noexcept;

    wsi_context& operator=(const wsi_context& rhs) = delete;
    wsi_context& operator=(wsi_context&& rhs) = delete;

    ptr<xcb_connection_t> connection();
    ptr<xcb_screen_t> default_screen();
    xcb_atom_t atom_by_name(const atom name) const;
    const std::string& instance_name() const;
    const std::string& application_name() const;
    xcb_window_t leader();
    ptr<xkb_context> keyboard_context();
    xcb_input_device_id_t keyboard();
    xcb_input_device_id_t pointer();
    bool is_keyboard_event(const u8 type) const;
  };

}

#endif
