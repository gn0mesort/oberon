#!/usr/bin/env python3

import sys
from pathlib import Path

VULKAN_U32_SIZE = 4

binary = bytes()
input_path = Path(sys.argv[2]).absolute()
output_path = Path(sys.argv[1]).absolute()
with open(input_path, 'rb') as file:
    binary = file.read()
var_name = input_path.name.replace(' ', '_').replace('.', '_').replace('-', '_').lower()

hex_str = ''
for i in range(0, len(binary), VULKAN_U32_SIZE):
    value = int.from_bytes(binary[i: i + VULKAN_U32_SIZE], byteorder='little', signed=False)
    hex_str += f'0x{value:08x}, '

header = """#ifndef OBERON_INTERNAL_SHADERS_{upper_name}_HPP
#define OBERON_INTERNAL_SHADERS_{upper_name}_HPP

#include <cinttypes>
#include <cstddef>

#include <array>

namespace oberon::internal::shaders {{

  constexpr const std::array<std::uint32_t, {size}> {name}{{ {data}}};

}}

#endif"""
header = header.format(name = var_name, upper_name = var_name.upper(), size = int(len(binary) / VULKAN_U32_SIZE),
                       data = hex_str)

with open(output_path, 'w', encoding = 'utf-8') as file:
    file.write(header)
