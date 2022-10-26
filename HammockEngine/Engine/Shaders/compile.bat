set COMPILER=C:/VulkanSDK/1.3.224.1/Bin/glslc.exe
set INPUT=.\Raw
set OUTPUT=.\Compiled

for /f %%f in ('dir /b .\Raw') do %COMPILER% %INPUT%\%%f -o %OUTPUT%\%%f.spv

pause