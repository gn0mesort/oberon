#ifndef OBERON_GRAPHICS_HPP
#define OBERON_GRAPHICS_HPP

#include <string>
#include <vector>

#include "types.hpp"
#include "memory.hpp"

#define OBERON_GRAPHICS_DEVICE_TYPES \
  OBERON_GRAPHICS_DEVICE_TYPE(integrated, 1) \
  OBERON_GRAPHICS_DEVICE_TYPE(discrete, 2) \
  OBERON_GRAPHICS_DEVICE_TYPE(virtualized, 3) \
  OBERON_GRAPHICS_DEVICE_TYPE(cpu, 4)

// These match the Vulkan spec
#define OBERON_PIPELINE_STAGES \
  OBERON_PIPELINE_STAGE(vertex, 0x1) \
  OBERON_PIPELINE_STAGE(fragment, 0x10)

#define OBERON_BUFFER_MODES \
  OBERON_BUFFER_MODE(double_buffer, 2) \
  OBERON_BUFFER_MODE(triple_buffer, 3)

namespace oberon {

#define OBERON_GRAPHICS_DEVICE_TYPE(name, value) name = (value),
  enum class graphics_device_type {
    other = 0,
    OBERON_GRAPHICS_DEVICE_TYPES
  };
#undef OBERON_GRAPHICS_DEVICE_TYPE

#define OBERON_PIPELINE_STAGE(name, value) name = (value),
  enum class pipeline_stage {
    none = 0,
    OBERON_PIPELINE_STAGES
  };
#undef OBERON_PIPELINE_STAGE

#define OBERON_BUFFER_MODE(name, value) name = (value),
  enum class buffer_mode {
    automatic = 0,
    OBERON_BUFFER_MODES
  };
#undef OBERON_BUFFER_MODE

  struct graphics_device final {
    graphics_device_type type{ };
    uptr handle{ };
    std::string name{ };
    std::string driver_name{ };
    std::string driver_info{ };
  };

  class graphics {
  public:
    graphics() = default;
    graphics(const graphics& other) = default;
    graphics(graphics&& other) = default;

    inline virtual ~graphics() noexcept = 0;

    graphics& operator=(const graphics& rhs) = default;
    graphics& operator=(graphics&& rhs) = default;


    virtual const std::vector<graphics_device>& available_devices() const = 0;
    virtual const graphics_device& preferred_device() const = 0;
    virtual void request_buffer_mode(const buffer_mode mode) = 0;
    virtual bool is_device_opened() const = 0;
    virtual void open_device(const graphics_device& device) = 0;
    virtual void close_device() = 0;
    virtual void wait_for_device_to_idle() = 0;
    virtual void dirty_renderer() = 0;
    virtual bool is_renderer_dirty() const = 0;
    virtual bool is_in_frame() const = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void draw_test_image() = 0;
  };

  graphics::~graphics() noexcept { }

  std::string to_string(const graphics_device_type type);
  std::string to_string(const pipeline_stage stage);

}

#endif
