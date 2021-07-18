#ifndef OBERON_DETAIL_BUILTIN_SHADERS_HPP
#define OBERON_DETAIL_BUILTIN_SHADERS_HPP

#include "../types.hpp"
#include "../memory.hpp"
#include "../debug.hpp"

#include "vulkan.hpp"

#define OBERON_GET_SHADER_STAGE_BINARY(name, stage, code, size) \
  do \
  {\
    auto result = oberon::detail::get_builtin_shader_binary<oberon::detail::builtin_shader_name::name, \
                                                            VK_SHADER_STAGE_##stage##_BIT>((code), (size)); \
    OBERON_ASSERT(!result); \
  } \
  while (0)

#define OBERON_GET_VERTEX_BINARY(name, code, size) \
  OBERON_GET_SHADER_STAGE_BINARY(name, VERTEX, code, size)

#define OBERON_GET_TESSELLATION_CONTROL_BINARY(name, code, size) \
  OBERON_GET_SHADER_STAGE_BINARY(name, TESSELLATION_CONTROL, code, size)

#define OBERON_GET_TESSELLATION_EVALUATION_BINARY(name, code, size) \
  OBERON_GET_SHADER_STAGE_BINARY(name, TESSELLATION_EVALUATION, code, size)

#define OBERON_GET_GEOMETRY_BINARY(name, code, size) \
  OBERON_GET_SHADER_STAGE_BINARY(name, GEOMETRY, code, size)

#define OBERON_GET_FRAGMENT_BINARY(name, code, size) \
  OBERON_GET_SHADER_STAGE_BINARY(name, FRAGMENT, code, size)

#define OBERON_GET_COMPUTE_BINARY(name, code, size) \
  OBERON_GET_SHADER_STAGE_BINARY(name, COMPUTE, code, size)

#define OBERON_BUILTIN_SHADERS \
  OBERON_BUILTIN_SHADER(test_frame, 0)

#define OBERON_BUILTIN_SHADER(name, value) \
  name = (value),

namespace oberon {
namespace detail {
  enum class builtin_shader_name {
    OBERON_BUILTIN_SHADERS
    max_value
  };

  const usize BUILTIN_SHADER_COUNT{ static_cast<usize>(builtin_shader_name::max_value) };

  template <builtin_shader_name Name, VkShaderStageFlagBits Stage>
  iresult get_builtin_shader_binary(readonly_ptr<u32>& code, usize& size) noexcept;
}
}

#undef OBERON_BUILTIN_SHADER

#endif
