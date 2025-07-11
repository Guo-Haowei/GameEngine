import os
import re
import subprocess
import platform

project_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
project_dir = os.path.abspath(project_dir)
print(project_dir)

clang_format_path = os.path.join(project_dir, 'bin/clang-format')
if platform.system() == 'Darwin':
    clang_format_path = 'clang-format'

skip_patterns = [
    'stb_image.h',
    'imgui_impl_*'
]

def need_format(file):
    # choose files
    file_ext = os.path.splitext(file)[1]
    if not (file_ext in ['.hpp', '.cpp', '.h', '.c', '.hlsl', '.mm', '.m', '.inl', '.glsl']):
        return False

    # white list
    file_name_with_ext = os.path.basename(file)
    for pattern in skip_patterns:
        if re.search(pattern, file_name_with_ext):
            return False
    return True

def format_folder(folder_name):
    for root, _, files in os.walk(folder_name, topdown=False):
        for name in files:
            file_path = os.path.join(root, name)
            if need_format(file_path):
                print(f'*** formatting file {file_path}')
                subprocess.run([clang_format_path, '-i', file_path])

for dir in ['engine', 'plugin', 'modules', 'applications']:
    format_folder(os.path.join(project_dir, dir))
