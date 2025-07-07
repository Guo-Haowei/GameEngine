/// File: vxgi.glsl
struct DiffuseCone {
    vec3 direction;
    float weight;
};

const DiffuseCone g_diffuse_cones[6] =
    DiffuseCone[6](DiffuseCone(vec3(0.0, 1.0, 0.0), MY_PI / 4.0), DiffuseCone(vec3(0.0, 0.5, 0.866025), 3.0 * MY_PI / 20.0),
                   DiffuseCone(vec3(0.823639, 0.5, 0.267617), 3.0 * MY_PI / 20.0),
                   DiffuseCone(vec3(0.509037, 0.5, -0.7006629), 3.0 * MY_PI / 20.0),
                   DiffuseCone(vec3(-0.50937, 0.5, -0.7006629), 3.0 * MY_PI / 20.0),
                   DiffuseCone(vec3(-0.823639, 0.5, 0.267617), 3.0 * MY_PI / 20.0));

vec3 trace_cones(sampler3D voxel, vec3 from, vec3 direction, float aperture) {
    float max_dist = 2.0 * c_voxelWorldSizeHalf;
    vec4 acc = vec4(0.0);

    float offset = 2.0 * c_voxelSize;
    float dist = offset + c_voxelSize;

    while (acc.a < 1.0 && dist < max_dist) {
        vec3 conePosition = from + direction * dist;
        float diameter = 2.0 * aperture * dist;
        float mipLevel = log2(diameter / c_voxelSize);

        vec3 coords = (conePosition - c_voxelWorldCenter) / c_voxelWorldSizeHalf;
        coords = 0.5 * coords + 0.5;

        vec4 voxel = textureLod(voxel, coords, mipLevel);
        acc += (1.0 - acc.a) * voxel;

        dist += 0.5 * diameter;
    }

    return acc.rgb;
}

vec3 cone_diffuse(sampler3D voxel, vec3 position, vec3 N) {
    const float aperture = 0.57735;

    vec3 diffuse = vec3(0.0);

    vec3 up = vec3(0.0, 1.0, 0.0);

    if (abs(dot(N, up)) > 0.999) up = vec3(0.0, 0.0, 1.0);

    vec3 T = normalize(up - dot(N, up) * N);
    vec3 B = cross(T, N);

    for (int i = 0; i < 6; ++i) {
        vec3 direction = T * g_diffuse_cones[i].direction.x + B * g_diffuse_cones[i].direction.z + N;
        direction = normalize(direction);

        diffuse += g_diffuse_cones[i].weight * trace_cones(voxel, position, direction, aperture);
    }

    return diffuse;
}

vec3 cone_specular(sampler3D voxel, vec3 position, vec3 direction, float roughness) {
    // TODO: brdf lookup
    float aperture = 0.0374533;

    aperture = clamp(tan(0.5 * MY_PI * roughness), aperture, 0.5 * MY_PI);

    vec3 specular = trace_cones(voxel, position, direction, aperture);

    return specular;
}
