#include <cstdio>

#include <unistd.h>

#include <oberon/errors.hpp>
#include <oberon/debug_context.hpp>

int main() {
  try
  {
    auto ctx = oberon::debug_context{
      "debug_context_create",
      1, 0, 0,
      { "VK_LAYER_KHRONOS_validation" }
    };
    sleep(2);
  }
  catch (const oberon::error& err)
  {
    std::fprintf(stderr, "%s\n", err.message());
    return err.result();
  }
  return 0;
}
