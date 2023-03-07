# oberon

A `+` indicates a requirement that I think is currently met. A `-` indicates something that is still incomplete.

## General

- *MUST* manage lifetimes of underlying system resources
- *MUST* encapsulate system resources to prevent incorrect operation
- *MUST* document all public symbols

## OS Support

- *MUST* support Linux
    + *MUST* be able to retrieve client machine name
    + *MUST* be able to retrieve PID

- *SHOULD* support other POSIX systems

- *SHOULD* support Windows

## Window System

+ *MUST* support programmatic window resizing
+ *MUST* support single window applications
    Q: Should a window be a separate object or be part of the graphics subsystem object?
    A: A window is created during system creation and is owned by the system object rather than a graphics object.
- *SHOULD* support multi-window applications
    Q: Does anyone actually do this?
    A: Some frameworks support it but I don't know of any game that supports a multiwindow rendering environment.
       Many support multi-monitor rendering but I don't think this is the same thing.
- *SHOULD* support display exclusive applications
    Q: What does display exclusive actually mean?
    A: Under X11 it seems to mean disabling the compositor and setting `_NET_WM_FULLSCREEN`, `_NET_WM_ABOVE`.
       I still don't know with regard to Windows or elsewhere. This doesn't seem to be the point of
       `VK_EXT_acquire_xlib_display`. EWMH seems to support a fullscreen window across 4 monitors (or maybe more if
       they are arranged in a rectangle).
+ *MUST* support keyboard
+ *MUST* support mouse input
- *SHOULD* support gamepad input
+ *MUST* support managed fullscreen windows

- *MUST* support X11
    Q: Should XInitThreads be called before XOpenDisplay
    A: For the sake of sanity, yes.
    + *MUST* support `RESOURCE_NAME`
    + *MUST* support `-name` argument
    + *MUST* properly set `WM_PROTOCOLS`
    + *MUST* handle `WM_DELETE_WINDOW` messages
    + *MUST* handle `_NET_WM_PING` messages
    + *MUST* properly set `WM_CLIENT_MACHINE`
    + *MUST* properly set `WM_CLASS`
    + *MUST* properly set `_NET_WM_PID`
    + *MUST* properly set `WM_NAME` and `_NET_WM_NAME`
    + *MUST* properly set `WM_NORMAL_HINTS`
    + *MUST* properly set `_NET_WM_STATE`
    + *MUST* support both Xlib and XCB
    + *MUST* support `_NET_WM_STATE` client messages to take fullscreen
    + *MUST* support `_NET_WM_STATE` client messages to release fullscreen
    + *MUST* support window resizing via menus/input
    + *SHOULD* support window resizing via window manager
    + *MUST* support US layout keyboard via events
    + *MUST* support mice via events
    - *SHOULD* support non-US keyboard layouts

- *SHOULD* support Win32

## Hardware Accelerated Graphics

- *MUST* use Vulkan 1.3
    + *MUST* support debug output
    + *MUST* support validation features and layers
    + *MUST* support manual device selection
    - *SHOULD* support later Vulkan versions
    - *SHOULD* support earlier Vulkan versions
    - *MUST* support single device rendering
    - *MUST* support single VkQueue operation
    - *SHOULD* support multi-VkQueue operation
    - *MUST* support rendering to window surface
        + *MUST* support rendering to XCB surfaces
        - *SHOULD* support rendering to Win32 surfaces
    - *MUST* support VkSwapchainKHR
    - *SHOULD* support rendering to off screen images
        - *SHOULD* support rendering to off screen images with different resolutions from swapchain images
    - *MUST* support rendering to color attachments
    - *MUST* support rendering to depth attachments
    - *MUST* support device memory allocation
    - *SHOULD* provide a custom device memory allocator
