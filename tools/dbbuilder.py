#!/usr/bin/env python3

import sqlite3
import json
import subprocess

from uuid import uuid4
from pathlib import Path
from argparse import ArgumentParser
from dbschema import db_schema

def path_to_private_name(path: Path, extension: str):
    safe_path = (path.parent / f'{path.stem}.{extension}').as_posix()
    return safe_path.replace('/', '_').replace('.._', '')


def compile_modules(compiler: Path, compiler_opts: str, module_dir: Path, metadata, sources: list[Path]):
    args = [ str(compiler) ] + compiler_opts.split(',')
    for source in sources:
        object_name = path_to_private_name(source.relative_to('.'), 'spv')
        module_path = module_dir / object_name
        subprocess.run(args + [ '-o', str(module_path), str(source) ], check=True)
        metadata['stages'][source.name]['module_path'] = module_path

def version_from_str(version: str):
    vals = version.split('.')
    return (int(vals[0]) << 22) | (int(vals[1]) << 12) | int(vals[2])

def insert_shader(db: sqlite3.Connection, metadata):
    name = metadata['name']
    version = version_from_str(metadata['version'])
    author = metadata['author'] if 'author' in metadata else 'unknown'
    description = metadata['description'] if 'description' in metadata else 'unspecified'
    license = metadata['license'] if 'license' in metadata else 'unspecified'
    db.execute('insert into shaders values (?, ?, ?, ?, ?);', [ name, version, author, description, license ])

def insert_modules(db: sqlite3.Connection, metadata):
    for stage_name in metadata['stages']:
        stage = metadata['stages'][stage_name]
        module = metadata['stages'][stage_name]['module_path'].read_bytes()
        module_uuid = uuid4().bytes
        stage['uuid'] = module_uuid
        db.execute('insert into modules values (?, ?);', [ module_uuid, module ])

def stage_name_to_type(name: str):
    mapping = { 'vertex': 0x1,
                'tessellation_control': 0x2,
                'tessellation_evaluation': 0x4,
                'geometry': 0x8,
                'fragment': 0x10,
                'compute': 0x20
              }
    return mapping[name]

def insert_stages(db: sqlite3.Connection, metadata):
    for stage_name in metadata['stages']:
        stage = metadata['stages'][stage_name]
        stage_type = stage_name_to_type(stage['type'])
        name = metadata['name']
        version = version_from_str(metadata['version'])
        module_uuid = stage['uuid']
        entry_point = stage['entry']
        db.execute('insert into stages values (?, ?, ?, ?, ?)', [ stage_type, name, version, module_uuid, entry_point ])

parser = ArgumentParser(description='Builds shader libraries.')
parser.add_argument('metafile', type=str, metavar='METAFILE', help='The path to a valid JSON metadata file describing the a shader.')
parser.add_argument('sources', type=str, metavar='SOURCES', nargs='+')
parser.add_argument('-m', '--mode', dest='mode', type=str, choices=[ 'init', 'update' ], default='update')
parser.add_argument('--compiler', dest='compiler', type=str, default='glslangValidator')
parser.add_argument('--compiler-opts', dest='compiler_opts', type=str, default='--target-env,vulkan1.2,-t,--quiet')
parser.add_argument('-o', '--output', dest='output', type=str, default='shaders.lib')
parser.add_argument('-M', '--module-dir', dest='module_dir', type=str, default='.')
parser.add_argument('-v', '--version', action='version', version='1.0.0')

args = parser.parse_args()

metafile = Path(args.metafile)
sources = [ Path(x) for x in args.sources ]
compiler = Path(args.compiler)
compiler_opts = args.compiler_opts
module_dir = Path(args.module_dir)
module_dir.mkdir(parents=True, exist_ok=True)
output = Path(args.output)
db = None
if not output.is_file() or args.mode == 'init':
    db = sqlite3.connect(output)
    db.executescript(db_schema)
else:
    db = sqlite3.connect(output)
metadata = json.loads(metafile.read_text('utf-8'))
compile_modules(compiler, compiler_opts, module_dir, metadata, sources)
insert_shader(db, metadata)
insert_modules(db, metadata)
insert_stages(db, metadata)
db.commit()
db.close()
