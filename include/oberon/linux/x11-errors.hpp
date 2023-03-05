/**
 * @file x11-errors.hpp
 * @brief X11 protocol error handling.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_LINUX_X11_ERRORS_HPP
#define OBERON_LINUX_X11_ERRORS_HPP

#include <string>

#include "../errors.hpp"

/**
 * @def OBERON_LINUX_X_ERRORS
 * @brief A list errors defined by the X11 protocol.
 */
#define OBERON_LINUX_X_ERRORS \
  OBERON_LINUX_X_ERROR(request, "The major or minor opcode of does not specify a valid request", 1) \
  OBERON_LINUX_X_ERROR(value, "Some numeric value falls outside the range of values accepted by the request." \
                       "Unless a specific range is specified for an argument, the full range defined by the " \
                       "argument's type is accepted. Any argument defind as a set of alternatives can generate this " \
                       "error (due to the encoding).", 2) \
  OBERON_LINUX_X_ERROR(window, "A value for a WINDOW argument does not name a defined WINDOW.", 3) \
  OBERON_LINUX_X_ERROR(pixmap, "A value for a PIXMAP argument does not name a defined PIXMAP.", 4) \
  OBERON_LINUX_X_ERROR(atom, "A value for an ATOM argument does not name a defined ATOM.", 5) \
  OBERON_LINUX_X_ERROR(cursor, "A value for a CURSOR argument does not name a defined CURSOR.", 6) \
  OBERON_LINUX_X_ERROR(font, "A value for a FONT argument does not name a defined FONT. " \
                       "A value for a FONTABLE arugment does not name a defined FONT or GCONTEXT." , 7) \
  OBERON_LINUX_X_ERROR(match, "An InputOnly window is used as a DRAWABLE. In a graphics request, the GCONTEXT " \
                       "argument does not have the same root and depth as the destination DRAWABLE argument. " \
                       "Some argument (or pair of arguments) has the correct type and range but it fails to match " \
                       "in some other way required by the request.", 8) \
  OBERON_LINUX_X_ERROR(drawable, "A value for a DRAWABLE argument does not name a defined WINDOW or PIXMAP.", 9) \
  OBERON_LINUX_X_ERROR(access, "An attempt is made to grab a key/button combination already grabbed by another " \
                       "client. An attempt is made to free a colormap entry not allocated by the client or to free " \
                       "an entry in a colormap that was created with all entries writable. An attempt is made to " \
                       "into a read-only or an unallocated colormap entry. An attempt is made to modify the access " \
                       "control list from other than the local host (or otherwise an authorized client). An attempt " \
                       "is made to select an event type that only one client can select at a time when another " \
                       "client has already selected it.", 10) \
  OBERON_LINUX_X_ERROR(alloc, "The server failed to allocate the requested resource. Note that the explicit listing " \
                       "of Alloc errors in request only covers allocation errors at a very coarse level and is not " \
                       "intended to cover all cases of a server running out of allocation space in the middle of " \
                       "service. The semantics when a server runs out of allocation space are left unspecified, " \
                       "but a server may generate an Alloc error on any request for this reason, and clients " \
                       "should be prepared to receive such errors and handle or discard them.", 11) \
  OBERON_LINUX_X_ERROR(colormap, "A value for a COLORMAP argument does not name a defined COLORMAP.", 12) \
  OBERON_LINUX_X_ERROR(gcontext, "A value for a GCONTEXT argument does not name a defined GCONTEXT.", 13) \
  OBERON_LINUX_X_ERROR(idchoice, "The value chosen for a resource identifier either is not included in the range " \
                       "assigned to the client or is already in use.", 14) \
  OBERON_LINUX_X_ERROR(name, "A font or color of the specified name does not exist.", 15) \
  OBERON_LINUX_X_ERROR(length, "The length of a request is shorter or longer than that required to minimally " \
                       "contain the arguments. The length of a request exceeds the maximum length accepted by the " \
                       "server.", 16) \
  OBERON_LINUX_X_ERROR(implementation, "The server does not implement some aspect of the request. A server that " \
                       "generates this error for a core request is deficient. As such, this error is not listed " \
                       "for any of the requests, but clients should be prepared to receive such errors and handle " \
                       "or discard them.", 17)

// No one defines this :(
/**
 * @def XCB_ERROR
 * @brief The xcb_generic_event_t::response_type indicating an error.
 */
#define XCB_ERROR 0

#if OBERON_CHECKS_ENABLED
  /**
   * @def OBERON_LINUX_X_SUCCEEDS
   * @brief Assign the return value of reqexp to target if reqexp succeeds.
   * @details This simplifies (somewhat) XCB error handling. For requests that do not send errors to the event queue.
   *          Importantly, it provides a value (err) that should be passed to the XCB reply function to retrieve the
   *          possible error. In the case that an error occurs, an x_error is thrown indicating the corresponding
   *          reqexp failed. In the case that the reply is successfully received, target is assigned a pointer to the
   *          reply. In this case the reply must be freed by the client code.
   * @param target The target value to assign to on success.
   * @param reqexp The request expression that will generate the necessary reply.
   */
  #define OBERON_LINUX_X_SUCCEEDS(target, reqexp) \
    do \
    { \
      auto err_ptr = oberon::ptr<xcb_generic_error_t>{ }; \
      auto err = &err_ptr; \
      (target) = (reqexp); \
      if (!(target)) \
      { \
        auto code = err_ptr->error_code; \
        std::free(err_ptr); \
        throw oberon::linux::x_error{ "Failed to get reply for \"" #reqexp "\".\n" + oberon::linux::to_string(static_cast<x_error_code>(code)), code }; \
      } \
    } \
    while (0)
#else
  #define OBERON_LINUX_X_SUCCEEDS(target, reqexp) \
    do \
    { \
      auto err = oberon::ptr<oberon::ptr<xcb_generic_error_t>>{ nullptr }; \
      (target) = (reqexp); \
    } \
    while (0)
#endif

namespace oberon::linux {

  /**
   * @brief An exception representing X11 errors.
   */
  OBERON_DYNAMIC_EXCEPTION_TYPE(x_error);

/// @cond
#define OBERON_LINUX_X_ERROR(name, msg, code) name = code,
/// @endcond

  enum class x_error_code {
    OBERON_LINUX_X_ERRORS
  };

/// @cond
#undef OBERON_LINUX_X_ERROR
/// @endcond

  /**
   * @brief Get the string representation of an X11 error code.
   * @param code The X11 error code to get string representation of.
   * @return A message, as provided by the X11 protocol specification, explaining the nature of the error code.
   */
  std::string to_string(const x_error_code code);

}

#endif
