#include "oberon/graphics.hpp"

namespace oberon {

  std::string to_string(const graphics_device_type type) {
#define OBERON_GRAPHICS_DEVICE_TYPE(name, value) case oberon::graphics_device_type::name: return (#name);
    switch (type)
    {
    OBERON_GRAPHICS_DEVICE_TYPES
    default:
      return "other";
    }
#undef OBERON_GRAPHICS_DEVICE_TYPE
  }

  std::string to_string(const pipeline_stage stage) {
#define OBERON_PIPELINE_STAGE(name, value) case oberon::pipeline_stage::name: return (#name);
    switch (stage)
    {
    OBERON_PIPELINE_STAGES
    default:
      return "none";
    }
#undef OBERON_PIPELINE_STAGE
  }

}
