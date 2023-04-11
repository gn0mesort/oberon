#ifndef OBERON_INTERNAL_LINUX_XCB_WSI_CONTEXT_HPP
#define OBERON_INTERNAL_LINUX_XCB_WSI_CONTEXT_HPP

#include "../../../memory.hpp"

#include "xcb.hpp"

namespace oberon::internal {

  class wsi_context final {
  private:
    ptr<xcb_connection_t> m_connection{ };
    readonly_ptr<xcb_setup_t> m_setup{ };
    ptr<xcb_screen_t> m_default_screen{ };
  public:
    wsi_context();
    wsi_context(const cstring displayname);
    wsi_context(const wsi_context& other) = delete;
    wsi_context(wsi_context&& other) = delete;

    ~wsi_context() noexcept;

    wsi_context& operator=(const wsi_context& rhs) = delete;
    wsi_context& operator=(wsi_context&& rhs) = delete;

    ptr<xcb_connection_t> connection();
    ptr<xcb_screen_t> default_screen();
  };

};

#endif
