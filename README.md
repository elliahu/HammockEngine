![Hammock Engine Logo](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/hammock-engine-logo.png)

# Hammock Engine 🔥
Custom c++20 game engine using Vulkan - the next generation graphics API.

# Features 🚀
The following points are features currently implemented:
- [x] Collision detection using AABB
- [x] Basic UI system
- [x] PBR (still improving, mainly descriptor binding)
- [x] Deferred renderer
- [x] glTF 2.0 support (still improving, mainly descriptor binding)
- [x] Shadow mapping (still improving, currently only singular directional/spot source)
- [x] SSAO
- [x] Reflections and Cubemaps
- [x] Volumetric rendering and physically accurate clouds
- [x] Partial IBL support (needs precomputed maps)

# TODOS ⏱
- [ ] Indirect drawing 
- [ ] Asset system and audio support
- [ ] Physics and collisions
- [ ] Optional RTX features
- Will see what more

## Currently working on (hot stuff)
- Implementing more robust shadow system
- Plug-in system for render systems to expose and sample from frame buffer attachments

## Engine dependencies 📚
The following NuGet packages are used:
- [GLM](https://github.com/g-truc/glm) - Linear algebra lib for matrix and vector operations (may switch to handmade math later as it is much lighter)
- [GLFW](https://www.glfw.org/) - Window creation and input handling

![Sponza](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/sponza.png)
![Sponza2](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/sponza2.png)
![Sponza3](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/sponza3.png)
![Sponza4](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/sponza4.png)
### Screen space ambient occlusion
![SSAO](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/ssao.png)
### glTF 2.0 model
![glTF 2.0](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/IBL.png)
### Vulumetrics
![Volumetrics](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/clouds.png)


