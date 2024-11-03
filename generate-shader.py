import os
import re
import shutil
import subprocess
import sys

project_dir = os.path.dirname(os.path.abspath(__file__))
generated_dir = 'source/shader/glsl_generated'

dxc_path = os.path.join(project_dir, 'bin/dxc')
spriv_cross_path = os.path.join(project_dir, 'bin/spirv-cross')

input_shaders = [
    { 'path': 'shadowmap_point.vert', 'animated': True, },
    { 'path': 'shadowmap_point.pixel', 'animated': False, },
    { 'path': 'bloom_setup.comp', 'animated': False, },
    { 'path': 'bloom_downsample.comp', 'animated': False, },
    { 'path': 'bloom_upsample.comp', 'animated': False, },
    { 'path': 'particle_initialization.comp', 'animated': False, },
    { 'path': 'particle_kickoff.comp', 'animated': False, },
    { 'path': 'particle_emission.comp', 'animated': False, },
    { 'path': 'particle_simulation.comp', 'animated': False, },
]

def generate(hlsl_source, animated):
    full_input_path = f'source/shader/hlsl/{hlsl_source}.hlsl'
    # shader model
    shader_model = ''
    if hlsl_source.endswith('.vert'):
        shader_model = 'vs_6_0'
    elif hlsl_source.endswith('.pixel'):
        shader_model = 'ps_6_0'
    elif hlsl_source.endswith('.comp'):
        shader_model = 'cs_6_0'
    else:
        raise RuntimeError(f'Unknown shader type "{full_input_path}"')

    # output path
    glsl_file = f'{generated_dir}/{hlsl_source}.glsl'

    # generate shader
    output_spv = 'tmp.spv'
    include_path = 'source/shader/'

    spv_command = [dxc_path, full_input_path, '-T', shader_model, '-E', 'main', '-Fo', output_spv, '-spirv', '-I', include_path, '-D HLSL_LANG=1']
    generate_files = [ { 'filename': glsl_file, 'command' : spv_command } ]
    if animated:
        new_command = spv_command + ['-D HAS_ANIMATION=1']
        if not hlsl_source.endswith('.vert'):
            raise RuntimeError(f'"{full_input_path}" is not vertex shader')
        new_path = f'source/shader/glsl_generated/animated_{hlsl_source}.glsl'
        generate_files.append({ 'filename': new_path, 'command': new_command })

    for generate_file in generate_files:
        print(f'running: {' '.join(generate_file['command'])}')
        subprocess.run(generate_file['command'])
        subprocess.run([spriv_cross_path, output_spv, '--version', '450', '--output', generate_file['filename']])
        print(f'file "{generate_file['filename']}" generated')
    return

def delete_and_create_folder(folder):
    if os.path.exists(folder):
        print(f'cleaning folder {folder}')
        shutil.rmtree(folder)
    os.makedirs(folder)

try:
    # remove generated path
    delete_and_create_folder(generated_dir)

    for l in input_shaders:
        generate(l['path'], l['animated'])
except RuntimeError as e:
    print(f'RuntimeError: {e}')
    sys.exit(1)
finally:
    # delete .spv
    os.remove('tmp.spv')