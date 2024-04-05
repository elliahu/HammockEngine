# Build glsl shaders to SPIR-V format

import os
import subprocess

compiler = os.environ['VULKAN_SDK'] + '\Bin\glslc.exe'
input = 'Raw\glsl'
output = 'Compiled'

# Iterate shaders
for path in os.listdir(input):
    shader_in = os.path.abspath(os.path.join(input, path))
    shader_out = os.path.abspath(os.path.join(output, path)) + '.spv'
    # Check if current path is a file
    if os.path.isfile(os.path.join(input, path)):
        #compile
        subprocess.check_call([compiler, shader_in, '-o', shader_out])
        print(path + ' compiled successfully')
