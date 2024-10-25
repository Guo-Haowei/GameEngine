import os
import re
import subprocess

project_dir = os.path.dirname(os.path.abspath(__file__))
dxc_path = os.path.join(project_dir, 'bin/dxc')

input_shader = 'source/shader/hlsl/bloom_setup.comp.hlsl'
output_spirv = 'bloom_setup.comp.spv'
include_path = 'source/shader/'
subprocess.run([dxc_path, input_shader, '-T cs_6_0', '-E main', '-Fo', output_spirv, '-spirv', '-I', include_path, '-D HLSL_LANG=1'])
