#ifndef OBERON_GRAPHICS_HPP
#define OBERON_GRAPHICS_HPP

#include <string>
#include <vector>
#include <ranges>
#include <iterator>

#include "types.hpp"
#include "memory.hpp"

#define OBERON_GRAPHICS_DEVICE_TYPES \
  OBERON_GRAPHICS_DEVICE_TYPE(other, 0) \
  OBERON_GRAPHICS_DEVICE_TYPE(integrated, 1) \
  OBERON_GRAPHICS_DEVICE_TYPE(discrete, 2) \
  OBERON_GRAPHICS_DEVICE_TYPE(virtualized, 3) \
  OBERON_GRAPHICS_DEVICE_TYPE(cpu, 4)

// These match the Vulkan spec
#define OBERON_PIPELINE_STAGES \
  OBERON_PIPELINE_STAGE(vertex, 0x1) \
  OBERON_PIPELINE_STAGE(fragment, 0x10)

namespace oberon {

#define OBERON_GRAPHICS_DEVICE_TYPE(name, value) name = (value),
  enum class graphics_device_type {
    OBERON_GRAPHICS_DEVICE_TYPES
  };
#undef OBERON_GRAPHICS_DEVICE_TYPE

#define OBERON_PIPELINE_STAGE(name, value) name = (value),
  enum class pipeline_stage {
    none = 0,
    OBERON_PIPELINE_STAGES
  };
#undef OBERON_PIPELINE_STAGE

  struct graphics_device final {
    graphics_device_type type{ };
    uptr handle{ };
    std::string name{ };
    std::string driver_name{ };
    std::string driver_info{ };
  };

  struct pipeline_stage_binary final {
    pipeline_stage stage{ };
    usize id{ };
  };

  struct render_program final {
    usize vertex_stage_id{ };
    usize fragment_stage_id{ };
  };

  class graphics {
  protected:
    virtual pipeline_stage_binary intern_pipeline_stage_binary(const pipeline_stage stage,
                                                               const readonly_ptr<char> bin, const usize sz) = 0;
  public:
    graphics() = default;
    graphics(const graphics& other) = default;
    graphics(graphics&& other) = default;

    inline virtual ~graphics() noexcept = 0;

    graphics& operator=(const graphics& rhs) = default;
    graphics& operator=(graphics&& rhs) = default;


    virtual const std::vector<graphics_device>& available_devices() const = 0;
    virtual void select_device(const graphics_device& device) = 0;
    virtual void wait_for_device_to_idle() = 0;
    virtual void reinitialize_renderer() = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    template <std::ranges::contiguous_range Type>
    pipeline_stage_binary intern_pipeline_stage_binary(const pipeline_stage stage, Type&& binary);
    virtual usize intern_render_program(const render_program& program) = 0;
    virtual void draw(const usize vertices, const usize program) = 0;
  };

  graphics::~graphics() noexcept { }

  std::string to_string(const graphics_device_type type);
  std::string to_string(const pipeline_stage stage);

  template <std::ranges::contiguous_range Type>
  pipeline_stage_binary graphics::intern_pipeline_stage_binary(const pipeline_stage stage, Type&& binary) {
    using value_type = typename std::iterator_traits<decltype(std::begin(binary))>::value_type;
    return intern_pipeline_stage_binary(stage, std::data(binary), std::size(binary) * sizeof(value_type));
  }

}

#endif
