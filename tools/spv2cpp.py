#!/usr/bin/env python3

from pathlib import Path
from argparse import ArgumentParser

#
#def compile_module(compiler: str, args: list[str], source: Path, output: Path):
#    cmd = [ compiler, '-t', '-Os', '-g0', '--target-env', 'vulkan1.2', '--quiet', '-o', str(output.resolve()) ]
#    cmd += args
#    cmd += [ str(source.resolve(strict=True)) ]
#    subprocess.run(cmd, check=True)
#    return output.read_bytes()

def format_binary(name: str, binary: bytes):
    binary_str = ''
    for b in binary:
        binary_str += f'0x{b:02x},'
    return f"""
  static const std::array<uchar, {len(binary)}> sg_{name}{{ {binary_str} }};
"""

def stage_name_from_path(src: Path):
    parts = src.stem.split('.')
    stage_name = parts[-1]
    if parts[-1] == 'glsl' or parts[-1] == 'hlsl':
        stage_name = parts[-2]
    return stage_name

def stage_bit_from_name(stage_name: str):
    if stage_name == 'vert':
        return 'VK_SHADER_STAGE_VERTEX_BIT'
    elif stage_name == 'tesc':
        return 'VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT'
    elif stage_name == 'tese':
        return 'VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT'
    elif stage_name == 'geom':
        return 'VK_SHADER_STAGE_GEOMETRY_BIT'
    elif stage_name == 'frag':
        return 'VK_SHADER_STAGE_FRAGMENT_BIT'
    elif stage_name == 'comp':
        return 'VK_SHADER_STAGE_COMPUTE_BIT'
    return '0'

def format_template(name: str, array_name: str, stage: str):
    return f"""
  template <>
  iresult
  get_builtin_shader_binary<builtin_shader_name::{name}, {stage}>(readonly_ptr<u32>& code, usize& size) noexcept {{
    code = reinterpret_cast<readonly_ptr<u32>>(std::data(sg_{array_name}));
    size = std::size(sg_{array_name});
    return 0;
  }}
"""


SOURCE_FILE = """
#include "oberon/detail/builtin_shaders.hpp"

#include <array>

namespace {{
    using oberon::uchar;

    {binaries}

}}

namespace oberon {{
namespace detail {{

    {templates}

}}
}}
"""


parser = ArgumentParser(description='Builds a C++ source file from SPIR-V binaries.')
parser.add_argument('sources', metavar='SOURCES', nargs='+', type=str,
                    help='SPIR-V files to build C++ source from.')
parser.add_argument('--shader-name', dest='shader_name', type=str, default='',
                    help='The desired name of the resulting shader. If no name is provided it will be inferred.')
parser.add_argument('-o', '--output', type=str, default='.',
                    help='A path to write output to.')
parser.add_argument('-v', '--version', action='version', version='1.0.0',
                    help='Report version information.')
args = parser.parse_args()

binaries = ''
templates = ''
for src in [ Path(src) for src in args.sources ]:
    stage_name = stage_name_from_path(src)
    shader_name = args.shader_name if len(args.shader_name) > 0 else src.stem.replace('.', '_')
    array_name = f'{shader_name}_{stage_name}'
    binaries += format_binary(array_name, src.read_bytes())
    templates += format_template(shader_name, array_name, stage_bit_from_name(stage_name))

output_file = open(Path(args.output), 'w')
print(SOURCE_FILE.format(binaries=binaries, templates=templates), file=output_file)
output_file.close()
