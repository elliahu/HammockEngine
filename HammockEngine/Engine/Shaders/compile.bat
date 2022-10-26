set COMPILER=C:\VulkanSDK\1.3.224.1\Bin\glslc.exe
set INPUT=C:\Users\matej\Documents\Projects\Personal\HammockEngine\HammockEngine\Engine\Shaders\Raw
set OUTPUT=C:\Users\matej\Documents\Projects\Personal\HammockEngine\HammockEngine\Engine\Shaders\Compiled

for /f %%f in ('dir /b %INPUT%') do %COMPILER% %INPUT%\%%f -o %OUTPUT%\%%f.spv




EXIT 0