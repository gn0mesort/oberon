#!/usr/bin/env python3

import sqlite3

from pathlib import Path
from argparse import ArgumentParser
from dbschema import db_schema

db_merge = """
            begin;
            insert into shaders select * from to_merge.shaders;
            insert into modules select * from to_merge.modules;
            insert into stages select * from to_merge.stages;
            commit;
           """

parser = ArgumentParser(description='Merges shader libraries.')
parser.add_argument('dbs', metavar='DBS', type=str, nargs='+')
parser.add_argument('-o', '--output', dest='output', type=str, default='shaders.lib')
args = parser.parse_args()

output = Path(args.output)
db_paths = [ Path(db) for db in args.dbs ]

db = sqlite3.connect(output)
db.executescript(db_schema)
for merge_db in db_paths:
    if merge_db.is_file():
        db.execute('attach ? as to_merge;', [ str(merge_db) ])
        db.executescript(db_merge)
        db.execute('detach to_merge;')
db.commit()
db.close()
