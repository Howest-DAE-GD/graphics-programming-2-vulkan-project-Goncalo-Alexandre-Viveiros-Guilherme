### Code by Gonçalo Alexandre Viveiros Guilherme

# Introduction 
This is a small rendering framework for vulkan with the ability to load various gltf/obj/fbx objects, render them and to manually place lights within the scene.

![image](https://github.com/user-attachments/assets/5240120f-593c-4d2a-b499-1098e7d669d2)


# Main features:
This renderer is able to load multiple formats for 3D models including those with embedded textures. It also has a couple "fail safe" texture to hopefully allow you to load a model even without any textures.
It uses assimp and should be able to load a majority of the formats that assimp can.

glb:

![image](https://github.com/user-attachments/assets/b5ed84b9-e0db-4202-8e40-2ea11538f13c)


The project has deffered rendering with directional lights and point lights implemented. There is also on boot shader loading. So you can quickly re-load your scenes after making changes. It uses physical camera simulation.
Which can be setup within the blitShader. 

![image](https://github.com/user-attachments/assets/cbd1d041-731a-4f29-8311-b9ba9da559e4)


It has pbr texture support, for normals. metallic and roughness and obviously albedo (No AO support :/)

![image](https://github.com/user-attachments/assets/555bf061-019e-41f6-90a3-2d85a0b21551)

![image](https://github.com/user-attachments/assets/daabc0a2-7764-4ba0-9fb4-97f9705bdf9c)


# Conclusion
Well after that short main features chapter we already reached the conclusion. I had lots of fun working on this project! I might even keep working on it after this and hopefully add all the things I dint have time for.

Url to github: https://github.com/Howest-DAE-GD/graphics-programming-2-vulkan-project-Goncalo-Alexandre-Viveiros-Guilherme


