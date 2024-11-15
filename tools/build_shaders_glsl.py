# Build glsl shaders to SPIR-V format

import os
import subprocess

compiler = os.environ['VULKAN_SDK'] + '/Bin/glslc.exe'
raw_shaders = r'../src/engine/shaders/raw/glsl'
compiled_shaders = r'../src/engine/shaders/compiled'

# Iterate shaders
for path in os.listdir(raw_shaders):
    shader_in = os.path.abspath(os.path.join(raw_shaders, path))
    shader_out = os.path.abspath(os.path.join(compiled_shaders, path)) + '.spv'
    # Check if current path is a file
    if os.path.isfile(os.path.join(raw_shaders, path)):
        #compile
        subprocess.check_call([compiler, shader_in, '-o', shader_out, '--target-env=vulkan1.2', '--target-spv=spv1.4']) 
        print(path + ' compiled successfully')
