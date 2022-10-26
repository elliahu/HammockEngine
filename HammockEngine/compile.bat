set COMPILER=%VULKAN_SDK%\Bin\glslc.exe
set INPUT=.\Engine\Shaders\Raw
set OUTPUT=.\Engine\Shaders\Compiled

for /f %%f in ('dir /b %INPUT%') do %COMPILER% %INPUT%\%%f -o %OUTPUT%\%%f.spv

pause