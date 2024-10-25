import os
import re
import subprocess

project_dir = os.path.dirname(os.path.abspath(__file__))

dxc_path = os.path.join(project_dir, 'bin/dxc')
spriv_cross_path = os.path.join(project_dir, 'bin/spirv-cross')

input_shaders = [
    'source/shader/hlsl/bloom_setup.comp.hlsl',
    'source/shader/hlsl/bloom_downsample.comp.hlsl',
    'source/shader/hlsl/bloom_upsample.comp.hlsl',
]

for hlsl_source in input_shaders:
    output_spv = 'tmp.spv'
    include_path = 'source/shader/'
    subprocess.run([dxc_path, hlsl_source, '-T', 'cs_6_0', '-E', 'main', '-Fo', output_spv, '-spirv', '-I', include_path, '-D HLSL_LANG=1'])
    glsl_file = hlsl_source.replace('hlsl', 'glsl')
    print(f'generating file "{glsl_file}"')
    subprocess.run([spriv_cross_path, output_spv, '--version', '450', '--output', glsl_file])
