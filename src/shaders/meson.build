shader_srcs += custom_target('test_frame.cpp',
                             input: [ glslc.process(files('test_frame/test_frame.vert')),
                                      glslc.process(files('test_frame/test_frame.frag')) ],
                             output: 'test_frame.cpp',
                             command: [ spv2cpp, '--shader-name', 'test_frame', '-o', '@OUTPUT@', '@INPUT@' ])
