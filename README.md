![Hammock Engine Logo](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/hammock-engine-logo.png)

# Hammock Engine 🔥
Custom game engine using Vulkan API.

# Roadmap 🚗
The following points are milestones i would like to hit in near future:
- [x] Model loading
- [x] 3D transformations
- [x] Basic lighting (Blinn-Phong illumination model)
- [x] Collision detection using AABB (Axis-aligned Bounding Boxes)
- [x] Basic UI system
- [x] [Still improving] Materials, Textures and PBR
- [ ] Shadows (somewhat done, but not completely)
- [ ] Asset system and audio support
- [ ] Reflections and Cubemaps
- [ ] Physics and collisions
- Will see what more

## Currently working on (hot stuff)
- Implementing shadow system that will handle all kind of shadow types and will be performant. (basic singular-source spot/directional shadow already implemented)
- Code refactoring and rethinking architectural choices - mainly descriptor handling 
- Abstracting input into InputManager just like in SDL2 branch

## Engine dependencies 📚
The following libraries are used:
- [GLM](https://github.com/g-truc/glm) - Linear algebra (matrix and vector operations) 
- [GLFW](https://www.glfw.org/) - Window creation and input handling
- [TinyObjectLoader](https://github.com/tinyobjloader/tinyobjloader) - Excellent single header library for object loading
- [ImGUI](https://github.com/ocornut/imgui) - Graphical User Interface library
- [stb](https://github.com/nothings/stb)- stb_image.h for image loading
- [Assimp](https://github.com/assimp/assimp) - Asset import library, it will replace TinyObjectLoader when asset system is finished

![Textures](https://raw.githubusercontent.com/elliahu/HammockEngine/master/Img/pbr.png)

## My thoughts
I have decided to abandon SDL2 and I am gonna be using GLFW from now on even though it lack audio and media support. There are more reasons to this - but mainly it is that SDL is too heavy and doesn't always work as it should on modern hardware or W11.

