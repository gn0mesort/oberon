#include <unistd.h>

#include <oberon/debug_context.hpp>

int main() {
  auto ctx = oberon::debug_context{ { "VK_LAYER_KHRONOS_validation" } };
  sleep(2);
  return 0;
}
