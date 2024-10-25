#version 450
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 3) uniform writeonly image2D g_output_image;
uniform sampler2D SPIRV_Cross_Combinedg_bloom_input_imageu_sampler;

void main()
{
    uvec2 _37 = uvec2(imageSize(g_output_image));
    vec3 _53 = textureLod(SPIRV_Cross_Combinedg_bloom_input_imageu_sampler, vec2(float(gl_GlobalInvocationID.x) / float(_37.x), float(gl_GlobalInvocationID.y) / float(_37.y)), 0.0).xyz;
    imageStore(g_output_image, ivec2(gl_GlobalInvocationID.xy), mix(_53, vec3(0.0), bvec3(sqrt(dot(_53, vec3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625))) < 1.2999999523162841796875)).xyzz);
}

