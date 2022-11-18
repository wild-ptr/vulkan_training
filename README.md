# vulkan_training
This is my vulkan learning experience. Right now we have asset importing, some phong shading and working camera.

The build system is non-existent and works on a single Makefile because well... i hate build systems and im too lazy to use CMake for my personal fun project.

Soo, around 4K of my own LOC to view two triangles. But other features should progress much faster now :D

Features right now:
 - Full vulkan bootstraping
 - Higher level abstractions for vulkan objects and methods (framebuffer creation, etc)
 - Support for n UBO's per renderable (to run multiple frames in flight), that are kept in one VkBuffer,
   with automatic care for proper physical device limits and alignment requirements
 - SPIR-V shader reflection for automating pipeline creation, PipelineLayouts, and automation of creating
   DescriptorSetLayouts.
 - Right now the UBO system is two-tier, with set 0 being used for data changing per-frame and set 1 for
   data changing per-object. Facilities for pushing set0 are not implemented yet, as i have not implemented
   camera and by extension view and projection matrices cals.
 - Vertex abstractions.
  
  
Planned features:
 - Adding support for textures in bindless mode, to have another tier of uniforms with per-mesh rebind frequency.
   Probably implemented as push constants, as ideally i only want to pass texture sampler indices to shader.
 - Abstract the renderables and make some callback functions for them as to change their state. Right now renderable
   objects are completely static. See my opengl_training to see what i mean.
 - Adding support for UBO's binding only to fragment shader. Right now all uniform sets have to be declared in
   vertex shading, as its the only stage undergoing shader reflection now.
 - Adding assimp.
 - Rewrite of recording-presenting part to use framesInFlight command buffers instead of swapChainImages buffer.
 - Add support for staging buffer to push vertices and indices to GPU-only memory. Right now its in device-visible
   host-coherent (yuck!) (Will do this when implementing textures, as i can reuse staging abstractions from them)
 - full PBR IBL pipeline.
 - changing synchronization to Vulkan 1.2 timeline semaphores. Just for fun.
 
 


