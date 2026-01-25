/usr/local/bin/glslc ./src/prerender/shaders/shader.vert -o ./build/bin/vert.spv
/usr/local/bin/glslc ./src/prerender/shaders/shader.frag -o ./build/bin/frag.spv

/usr/local/bin/glslc ./src/lens/shaders/lens.vert -o ./build/bin/lens_vert.spv
/usr/local/bin/glslc ./src/lens/shaders/lens.frag -o ./build/bin/lens_frag.spv