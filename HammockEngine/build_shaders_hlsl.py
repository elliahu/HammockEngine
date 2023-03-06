# Build HLSL shaders to SPIR-V format
# TODO finish this to support all shader formats
import os
import subprocess

compiler = os.environ['VULKAN_SDK'] + '\Bin\dxc.exe'
input = 'Engine\Shaders\Raw\hlsl'
output = 'Engine\Shaders\Compiled'

# Iterate shaders
for path in os.listdir(input):
    shader_in = os.path.abspath(os.path.join(input, path))
    shader_out = os.path.abspath(os.path.join(output, path)) + '.spv'
    # Check if current path is a file
    if os.path.isfile(os.path.join(input, path)):
        #compile
        subprocess.check_call([compiler, '-spirv', '-T', 'vs_6_4', '-E', 'main', shader_in, '-Fo', shader_out])
        print(path + ' compiled successfully')
