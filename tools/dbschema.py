db_schema = """
                drop table if exists shaders;
                create table shaders (
                    name text not null,
                    version integer not null,
                    author text not null default 'unknown',
                    description text not null default 'unspecified',
                    license text not null default 'unspecified',

                    primary key (name, version)
                );
                drop table if exists modules;
                create table modules (
                    uuid blob not null,
                    code blob not null,

                    primary key(uuid)
                );
                drop table if exists stages;
                create table stages (
                    stage_type integer not null,
                    shader_name text not null,
                    shader_version integer not null,
                    module_uuid blob not null,
                    entry_point text not null default 'main',

                    foreign key(shader_name, shader_version) references shaders(name, version),
                    foreign key(module_uuid) references modules(uuid),
                    primary key(stage_type, shader_name, shader_version),

                    unique (module_uuid, entry_point)
                );
            """
