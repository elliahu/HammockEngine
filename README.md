![Hammock Engine Logo](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/hammock-engine-logo.png)

# Hammock Engine 🔥
Custom c++17 game engine using Vulkan - next generation graphics API.

# Roadmap 🚗
The following points are milestones i would like to hit in near future:
- [x] Model loading
- [x] 3D transformations
- [x] Basic lighting (Blinn-Phong illumination model)
- [x] Collision detection using AABB (Axis-aligned Bounding Boxes)
- [x] Basic UI system
- [x] PBR (still improving, mainly descriptor binding)
- [x] Deferred rendering
- [x] glTF 2.0 support (still improving, mainly descriptor binding)
- [ ] Shadows (still improving, currently only one directional/spot source)
- [ ] Indirect drawing 
- [ ] Asset system and audio support
- [ ] Reflections and Cubemaps
- [ ] Physics and collisions
- Will see what more

## Currently working on (hot stuff)
- Implementing shadow system that will handle all kind of shadow types and will be performant. (basic singular-source spot/directional shadow already implemented)
- Code refactoring and rethinking architectural choices - mainly descriptor handling 

## Engine dependencies 📚
The following NuGet packages are used:
- [GLM](https://github.com/g-truc/glm) - Linear algebra lib for matrix and vector operations (may switch to handmade math later as it is much lighter)
- [GLFW](https://www.glfw.org/) - Window creation and input handling

![Textures](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/pbr.png)
![Textures](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/pbr2.png)


