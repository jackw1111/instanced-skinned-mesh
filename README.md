# notes

- OpenGL will need access to GL_ARB_arrays_of_arrays whether that is directly from a modern version of
OpenGL that has core support, or as an extension in OpenGL3.3 (you can add extensions when generating the package on https://glad.dav1d.de )
- Each model is grouped together in groups of 20 models of the same type (hardware limitation as OpenGL only allows 1024 mat4's to be sent to the GPU through a uniform so we send Bones[20][50] which is 20 similar models with 50 bones maximum to the GPU, you can play around with this value to your preference but don't forget to change the allocation code in animated_shader.cpp to match)
- each 20 models have there unique bones generated, and they are sent to the gpu and drawn per instanced mesh.
- I recently started having trouble getting this to work with the default assimp available in the apt repo so I was using libassimp.5.0.0 built from source.
- the demo was pulled out of some other code so it might be abit messy
- requires glm