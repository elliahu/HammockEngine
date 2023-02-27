set COMPILER=%VULKAN_SDK%\Bin\dxc.exe
set INPUT=.\Engine\Shaders\Raw\hlsl
set OUTPUT=.\Engine\Shaders\Compiled

for /f %%f in ('dir /b %INPUT%') do %COMPILER% -spirv -T vs_6_4 -E main %INPUT%\%%f -Fo %OUTPUT%\%%f.spv

pause