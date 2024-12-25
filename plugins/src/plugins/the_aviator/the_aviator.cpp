//  @TODO: refactor
#include "engine/core/base/random.h"
#include "engine/math/color.h"
#include "engine/math/geometry.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/camera_controller.h"
#include "engine/scene/scene.h"
#include "engine/scene/scriptable_entity.h"

// @TODO: remove
#include "engine/math/matrix_transform.h"
#include "engine/scene/scene_serialization.h"

namespace std {

template<>
struct hash<my::Vector3f> {
    std::size_t operator()(const my::Vector3f& p_vec) const {
        std::hash<uint8_t> byte_hash;
        size_t result = 0;

        const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(&p_vec);

        // Hash each byte of the memory block
        for (size_t i = 0; i < sizeof(p_vec); ++i) {
            result ^= byte_hash(byte_ptr[i]) + 0x9e3779b9 + (result << 6) + (result >> 2);
        }

        return result;
    }
};

}  // namespace std

namespace my {

// @TODO:
// * cascaded shadow map
// * fog
// * 2D UI
// * draw elements and draw array
// * motion blur

using math::AABB;
using math::Color;

enum : uint32_t {
    COLLISION_BIT_PLAYER = BIT(1),
    COLLISION_BIT_ROCK = BIT(2),
    COLLISION_BIT_BATTERY = BIT(3),
};

#define USE_DEBUG_CAMERA NOT_IN_USE

class SceneCreator {
public:
    Scene* CreateScene();

private:
    void CreateMaterials(Scene* p_scene);
    void CreatePlane(Scene* p_scene);
    void CreateMeshEmitter(Scene* p_scene);
    void CreateOcean(Scene* p_scene, ecs::Entity p_earth);

    MeshComponent MakeOceanMesh(float p_radius,
                                float p_height,
                                int p_sectors,
                                int p_height_sector);

    ecs::Entity material_red;
    ecs::Entity material_white;
    ecs::Entity material_white_transparent;
    ecs::Entity material_dark_brown;
    ecs::Entity material_brown;
    ecs::Entity material_blue_transparent;
    ecs::Entity material_pink;
    ecs::Entity mesh_rock;
    ecs::Entity mesh_rock_patricle;

    static constexpr float ROCK_SIZE = 4.0f;
    static constexpr float BATTERY_SIZE = 2.0f;
    static constexpr float PARTICLE_SIZE = 1.0f;
    static constexpr int ROCK_POOL_SIZE = 16;
    static constexpr int BATTERY_POOL_SIZE = 32;
    static constexpr int ROCK_PARTICLE_POOL_SIZE = 12;
    static constexpr int BATTERY_PARTICLE_POOL_SIZE = 48;
    static constexpr float OCEAN_RADIUS = 240.0f;
    static constexpr int CLOUD_COUNT = 20;
    static constexpr float plane_height = 30.0f;

    static constexpr Color RED_COLOR = Color::Hex(0xCE190A);
    static constexpr Color WHITE_COLOR = Color::Hex(0XD8D0D1);
    static constexpr Color BROWN_COLOR = Color::Hex(0x59332E);
    static constexpr Color DARK_BROWN_COLOR = Color::Hex(0x23190F);
    static constexpr Color BLUE_COLOR = Color::Hex(0X10A8A3);
    static constexpr Color PINK_COLOR = Color::Hex(0xF5986E);

    static constexpr std::array<Vector3f, 8> COCKPIT_POINTS = {
        Vector3f(-4.f, +1.5f, +0.5f),  // A
        Vector3f(-4.f, +0.5f, +0.5f),  // B
        Vector3f(+4.f, -2.5f, +2.5f),  // C
        Vector3f(+4.f, +2.5f, +2.5f),  // D
        Vector3f(-4.f, +1.5f, -0.5f),  // E
        Vector3f(-4.f, +0.5f, -0.5f),  // F
        Vector3f(+4.f, -2.5f, -2.5f),  // G
        Vector3f(+4.f, +2.5f, -2.5f),  // H
    };
};

Scene* SceneCreator::CreateScene() {

    ecs::Entity::SetSeed();

    Scene* scene = new Scene;
    scene->m_physicsMode = PhysicsMode::COLLISION_DETECTION;

    auto root = scene->CreateTransformEntity("root");
    scene->m_root = root;

    Vector2i frame_size = DVAR_GET_IVEC2(resolution);
    // editor camera
    {
        auto editor_camera = scene->CreatePerspectiveCameraEntity("editor_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(editor_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0.0f, plane_height + 10.0f, 50.0f));
        camera->SetEditorCamera();
        scene->AttachChild(editor_camera, root);
    }
    // main camera
    {
        auto main_camera = scene->CreatePerspectiveCameraEntity("main_camera", frame_size.x, frame_size.y);
        scene->AttachChild(main_camera, root);

        auto camera = scene->GetComponent<PerspectiveCameraComponent>(main_camera);
        DEV_ASSERT(camera);

#if USING(USE_DEBUG_CAMERA)
        camera->SetPosition(Vector3f(0.0f, plane_height + 10.0f, 400.0f));
        camera->SetPrimary();
        class InGameDebugCameraController : public EditorCameraController {
        public:
            InGameDebugCameraController() {
                SetScrollSpeed(10.0f);
                SetMoveSpeed(30.0f);
            }
        };
        scene->Create<NativeScriptComponent>(main_camera).Bind<InGameDebugCameraController>();
#else
        camera->SetPosition(Vector3f(0.0f, plane_height + 10.0f, 80.0f));
        camera->SetPrimary();
        scene->Create<LuaScriptComponent>(main_camera)
            .SetClassName("Camera")
            .SetPath("@res://scripts/camera.lua");
#endif
    }

    // create light
    auto light = scene->CreateInfiniteLightEntity("sun");
    {
        scene->AttachChild(light, root);
        TransformComponent* transform = scene->GetComponent<TransformComponent>(light);
        transform->RotateX(Degree(-41.f));
        transform->RotateY(Degree(-1.f));
        transform->RotateZ(Degree(-30.f));
        LightComponent* light_component = scene->GetComponent<LightComponent>(light);
        light_component->SetCastShadow();
        light_component->SetShadowRegion();
        light_component->m_shadowRegion = AABB::FromCenterSize(Vector3f(0, 20, -40), Vector3f(90));
    }

    auto world = scene->CreateTransformEntity("world");
    {
        scene->AttachChild(world, root);
        auto transform = scene->GetComponent<TransformComponent>(world);
        transform->Translate(Vector3f(0.0f, -OCEAN_RADIUS, 0.0f));
    }

    auto earth = scene->CreateTransformEntity("earth");
    scene->AttachChild(earth, world);
    scene->Create<LuaScriptComponent>(earth)
        .SetClassName("Earth")
        .SetPath("@res://scripts/earth.lua");

    CreateMaterials(scene);
    CreatePlane(scene);
    CreateOcean(scene, earth);

    // battery mesh
    mesh_rock = scene->CreateMeshEntity("rock_mesh");
    mesh_rock_patricle = scene->CreateMeshEntity("rock_particle_mesh");
    auto battery_mesh = scene->CreateMeshEntity("battery_mesh");
    auto battery_particle_mesh = scene->CreateMeshEntity("battery_particle_mesh");
    auto battery_material = scene->CreateMaterialEntity("battery_material");
    {
        MeshComponent* mesh = scene->GetComponent<MeshComponent>(battery_mesh);
        *mesh = MakeTetrahedronMesh(BATTERY_SIZE);

        MaterialComponent* material = scene->GetComponent<MaterialComponent>(battery_material);
        material->baseColor = Vector4f(BLUE_COLOR.r, BLUE_COLOR.g, BLUE_COLOR.b, 1.0f);
        mesh->subsets[0].material_id = battery_material;
    }
    {
        MeshComponent* mesh = scene->GetComponent<MeshComponent>(battery_particle_mesh);
        *mesh = MakeTetrahedronMesh(PARTICLE_SIZE);
        mesh->subsets[0].material_id = battery_material;
    }
    // rock
    auto rock_material = scene->CreateMaterialEntity("rock_material");
    {
        MeshComponent* mesh = scene->GetComponent<MeshComponent>(mesh_rock);
        *mesh = MakeSphereMesh(ROCK_SIZE, 6, 6);

        MaterialComponent* material = scene->GetComponent<MaterialComponent>(rock_material);
        material->baseColor = Vector4f(RED_COLOR.r, RED_COLOR.g, RED_COLOR.b, 1.0f);
        mesh->subsets[0].material_id = rock_material;
    }
    {
        MeshComponent* mesh = scene->GetComponent<MeshComponent>(mesh_rock_patricle);
        *mesh = MakeTetrahedronMesh(PARTICLE_SIZE * 2.0f);
        mesh->subsets[0].material_id = rock_material;
    }

    CreateMeshEmitter(scene);

#pragma region SETUP_GAME_OBJECTS
    {
        auto generator = scene->CreateTransformEntity("generator");
        scene->AttachChild(generator, earth);
        for (int i = 0; i < ROCK_POOL_SIZE; ++i) {
            auto id = scene->CreateObjectEntity(std::format("rock_{}", i));
            ObjectComponent* object = scene->GetComponent<ObjectComponent>(id);
            object->meshId = mesh_rock;
            auto& rigid_body = scene->Create<RigidBodyComponent>(id)
                                   .InitSphere(0.5f * ROCK_SIZE);
            rigid_body.collisionType = COLLISION_BIT_ROCK;
            rigid_body.collisionMask = COLLISION_BIT_PLAYER;

            scene->Create<LuaScriptComponent>(id)
                .SetClassName("Rock")
                .SetPath("@res://scripts/rock.lua");
            scene->AttachChild(id, generator);
        }

        for (int i = 0; i < BATTERY_POOL_SIZE; ++i) {
            auto id = scene->CreateObjectEntity(std::format("battery_{}", i));
            ObjectComponent* object = scene->GetComponent<ObjectComponent>(id);
            object->meshId = battery_mesh;
            auto& rigid_body = scene->Create<RigidBodyComponent>(id)
                                   .InitSphere(0.5f * ROCK_SIZE);
            rigid_body.collisionType = COLLISION_BIT_BATTERY;
            rigid_body.collisionMask = COLLISION_BIT_PLAYER;

            scene->Create<LuaScriptComponent>(id)
                .SetClassName("Battery")
                .SetPath("@res://scripts/battery.lua");
            scene->AttachChild(id, generator);
        }

        auto particle_wrapper = scene->CreateTransformEntity("particles");
        scene->AttachChild(particle_wrapper, world);
        for (int i = 0; i < ROCK_PARTICLE_POOL_SIZE; ++i) {
            auto id = scene->CreateObjectEntity(std::format("rock_particle_{}", i));
            ObjectComponent* object = scene->GetComponent<ObjectComponent>(id);
            object->meshId = mesh_rock_patricle;
            scene->Create<LuaScriptComponent>(id)
                .SetClassName("Particle")
                .SetPath("@res://scripts/particle.lua");
            scene->AttachChild(id, particle_wrapper);
        }

        for (int i = 0; i < BATTERY_PARTICLE_POOL_SIZE; ++i) {
            auto id = scene->CreateObjectEntity(std::format("battery_particle_{}", i));
            ObjectComponent* object = scene->GetComponent<ObjectComponent>(id);
            object->meshId = battery_particle_mesh;
            scene->Create<LuaScriptComponent>(id)
                .SetClassName("Particle")
                .SetPath("@res://scripts/particle.lua");
            scene->AttachChild(id, particle_wrapper);
        }
    }
#pragma endregion SETUP_GAME_OBJECTS

#pragma region SETUP_SKY
    constexpr float TWO_PI = 2.0f * glm::pi<float>();
    auto create_cloud = [&](int p_index, ecs::Entity p_parent) {
        int block = 3 + static_cast<int>(glm::floor(Random::Float() * 3));
        for (int block_index = 0; block_index < block; ++block_index) {
            std::string name = std::format("block_{}_{}", p_index, block_index);
            const float scale = 0.6f + Random::Float() * 0.4f;
            auto cloud = scene->CreateCubeEntity(name, material_white, Vector3f(5.0f * scale));
            auto transform = scene->GetComponent<TransformComponent>(cloud);
            Vector3f position(block_index * 8.0f, Random::Float() * 6.0f, Random::Float() * 6.0f);
            Vector3f rotation(0.0f, TWO_PI * Random::Float(), TWO_PI * Random::Float());
            transform->Rotate(rotation);
            transform->SetTranslation(position);

            scene->AttachChild(cloud, p_parent);
        }
    };

    constexpr float step_angle = TWO_PI / CLOUD_COUNT;
    for (int cloud_index = 0; cloud_index < CLOUD_COUNT; ++cloud_index) {
        const float angle = step_angle * cloud_index;
        std::string name = std::format("cloud_{}", cloud_index);
        auto cloud = scene->CreateTransformEntity(name);
        scene->AttachChild(cloud, earth);

        auto transform = scene->GetComponent<TransformComponent>(cloud);
        const float x = glm::sin(angle) * (OCEAN_RADIUS + 40.0f);
        const float y = glm::cos(angle) * (OCEAN_RADIUS + 40.0f);
        const Radian rad(angle);
        // @TODO: NEG_UNIT_X
        // @TODO: NEG_UNIT_Y
        // @TODO: NEG_UNIT_Z
        Matrix4x4f matrix = math::Translate(Vector3f(x, y, -50.0f)) * math::Rotate(rad, Vector3f(0, 0, -1));
        transform->SetLocalTransform(matrix);
        create_cloud(cloud_index, cloud);
    }
#pragma endregion SETUP_SKY

    {
        auto id = scene->CreateEnvironmentEntity("environment");
        scene->AttachChild(id, root);

        auto* env = scene->GetComponent<EnvironmentComponent>(id);
        env->ambient.color = Color::Hex(0xf7d9aa).ToVector4f();
        env->sky.texturePath = "@res://images/ibl/aviator_sky.hdr";
    }

    return scene;
}

void SceneCreator::CreateMaterials(Scene* p_scene) {
    constexpr float default_roughness = 0.8f;
    constexpr float default_metallic = 0.2f;
    struct {
        ecs::Entity* entity;
        const char* name;
        Color color;
    } info[] = {
    // clang-format off
#define COLOR(a, b) \
    { &(material_##a), "material_" #a, b },
        COLOR(red, RED_COLOR)
        COLOR(white, WHITE_COLOR)
        COLOR(brown, BROWN_COLOR)
        COLOR(dark_brown, DARK_BROWN_COLOR)
        COLOR(pink, PINK_COLOR)
        COLOR(blue_transparent, Color(BLUE_COLOR.r, BLUE_COLOR.g, BLUE_COLOR.b, 0.8f))
        COLOR(white_transparent, Color(WHITE_COLOR.r, WHITE_COLOR.g, WHITE_COLOR.b, 0.5f))
#undef COLOR
    };
    // clang-format on
    for (int i = 0; i < array_length(info); ++i) {
        DEV_ASSERT(*info[i].entity == ecs::Entity::INVALID);
        auto entity = p_scene->CreateMaterialEntity(info[i].name);
        MaterialComponent* material = p_scene->GetComponent<MaterialComponent>(entity);
        material->baseColor = info[i].color.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
        *(info[i].entity) = entity;
    }
}

MeshComponent SceneCreator::MakeOceanMesh(float p_radius,
                                          float p_height,
                                          int p_sectors,
                                          int p_height_sector) {
    MeshComponent mesh;

    auto& indices = mesh.indices;
    constexpr float pi = glm::pi<float>();

    std::array<float, 2> heights = { 0.5f * p_height, -0.5f * p_height };

    // cylinder side
    float height_step = (float)p_height / p_height_sector;
    for (float y = heights[1]; y < heights[0]; y += height_step) {
        for (int index = 0; index < p_sectors; ++index) {
            uint32_t point_offset = (uint32_t)mesh.positions.size();

            float angle_1 = 2.0f * pi * index / p_sectors;
            float x_1 = p_radius * std::cos(angle_1);
            float z_1 = p_radius * std::sin(angle_1);
            float angle_2 = 2.0f * pi * ((index + 1) == p_sectors ? 0 : (index + 1)) / p_sectors;
            float x_2 = p_radius * std::cos(angle_2);
            float z_2 = p_radius * std::sin(angle_2);

            Vector3f point_1(x_1, y, z_1);
            Vector3f point_2(x_1, y + height_step, z_1);

            Vector3f point_3(x_2, y, z_2);
            Vector3f point_4(x_2, y + height_step, z_2);

            Vector3f AB = point_1 - point_2;
            Vector3f AC = point_1 - point_3;
            Vector3f normal = math::normalize(math::cross(AB, AC));

            mesh.positions.emplace_back(point_1);
            mesh.positions.emplace_back(point_2);
            mesh.positions.emplace_back(point_3);

            mesh.positions.emplace_back(point_2);
            mesh.positions.emplace_back(point_4);
            mesh.positions.emplace_back(point_3);

            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);

            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());

            const uint32_t a = point_offset + 0;
            const uint32_t b = point_offset + 1;
            const uint32_t c = point_offset + 2;
            const uint32_t d = point_offset + 3;
            const uint32_t e = point_offset + 4;
            const uint32_t f = point_offset + 5;

            indices.emplace_back(a);
            indices.emplace_back(b);
            indices.emplace_back(c);
            indices.emplace_back(d);
            indices.emplace_back(e);
            indices.emplace_back(f);
        }
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

void SceneCreator::CreateMeshEmitter(Scene* p_scene) {
    auto id = p_scene->CreateTransformEntity("mesh_emitter");
    auto& emitter = p_scene->Create<MeshEmitterComponent>(id);
    DEV_ASSERT(mesh_rock_patricle.IsValid());
    emitter.meshId = mesh_rock_patricle;
    emitter.flags |= MeshEmitterComponent::RECYCLE;

    emitter.vxRange = Vector2f(-3, +3);
    emitter.vyRange = Vector2f(-3, +3);
    emitter.vzRange = Vector2f(-3, +3);
    emitter.gravity = Vector3f(0, -4, 0);
    emitter.scale = 0.4f;
    emitter.maxMeshCount = 24;
    emitter.emissionPerFrame = 2;
    emitter.lifetimeRange = Vector2f(3.f, 5.f);

    p_scene->AttachChild(id);
}

void SceneCreator::CreateOcean(Scene* p_scene, ecs::Entity p_earth) {
    class OceanScript : public ScriptableEntity {
        struct Wave {
            float angle;
            float amp;
            float speed;
        };

        void OnCreate() override {
            const ObjectComponent* object = GetComponent<ObjectComponent>();
            DEV_ASSERT(object);
            const MeshComponent* mesh = m_scene->GetComponent<MeshComponent>(object->meshId);
            DEV_ASSERT(mesh);

            std::unordered_map<Vector3f, Wave> cache;

            for (const Vector3f& position : mesh->positions) {

                auto it = cache.find(position);
                if (it != cache.end()) {
                    m_waves.push_back(it->second);
                    continue;
                }

                Wave wave;
                wave.angle = Random::Float() * 2.0f * glm::pi<float>();
                wave.amp = 3.5f + Random::Float() * 3.5f;
                wave.speed = 6.f * (Random::Float() * 1.0f);
                m_waves.emplace_back(wave);
                cache[position] = wave;
            }
        }

        void OnUpdate(float p_timestep) override {
            const ObjectComponent* object = GetComponent<ObjectComponent>();
            DEV_ASSERT(object);
            const MeshComponent* mesh = m_scene->GetComponent<MeshComponent>(object->meshId);
            DEV_ASSERT(mesh);

            auto& positions = mesh->updatePositions;
            auto& normals = mesh->updateNormals;
            positions.clear();
            normals.clear();
            positions.reserve(mesh->positions.size());
            normals.reserve(mesh->normals.size());

            for (size_t idx = 0; idx < mesh->positions.size(); idx += 3) {
                Vector3f points[3];
                for (int i = 0; i < 3; ++i) {
                    Wave& wave = m_waves[idx + i];
                    const Vector3f& position = mesh->positions[idx + i];
                    Vector3f new_position = position;
                    new_position.x += glm::cos(wave.angle) * wave.amp;
                    new_position.z += glm::sin(wave.angle) * wave.amp;
                    positions.emplace_back(new_position);
                    wave.angle += p_timestep * wave.speed;

                    points[i] = new_position;
                }

                Vector3f AB = points[0] - points[1];
                Vector3f AC = points[0] - points[2];
                Vector3f normal = math::normalize(math::cross(AB, AC));
                normals.emplace_back(normal);
                normals.emplace_back(normal);
                normals.emplace_back(normal);
            }
        }

        std::vector<Wave> m_waves;
    };

    auto ocean = p_scene->CreateMeshEntity("ocean", material_blue_transparent, MakeOceanMesh(OCEAN_RADIUS, 320.0f, 60, 16));
    ObjectComponent* object = p_scene->GetComponent<ObjectComponent>(ocean);
    object->flags |= ObjectComponent::FLAG_TRANSPARENT;

    DEV_ASSERT(object);

    MeshComponent* mesh = p_scene->GetComponent<MeshComponent>(object->meshId);
    DEV_ASSERT(mesh);
    mesh->flags |= MeshComponent::DYNAMIC;

    auto transform = p_scene->GetComponent<TransformComponent>(ocean);
    DEV_ASSERT(transform);
    transform->RotateX(Degree(90.0f));
    transform->RotateZ(Degree(90.0f));
    p_scene->AttachChild(ocean, p_earth);

    p_scene->Create<NativeScriptComponent>(ocean).Bind<OceanScript>();
}

void SceneCreator::CreatePlane(Scene* p_scene) {
    auto root = p_scene->m_root;
    auto plane = p_scene->CreateTransformEntity("plane");
    {
        TransformComponent* transform = p_scene->GetComponent<TransformComponent>(plane);
        transform->Translate(Vector3f(0.0f, plane_height, 0.0f));

        // @TODO: fix collision boundary, probably want to visualize it
        auto& rigid_body = p_scene->Create<RigidBodyComponent>(plane)
                               .InitSphere(4.0f);

        rigid_body.collisionType = COLLISION_BIT_PLAYER;
        rigid_body.collisionMask = COLLISION_BIT_BATTERY | COLLISION_BIT_ROCK;

        p_scene->AttachChild(plane, root);
        p_scene->Create<LuaScriptComponent>(plane)
            .SetClassName("Plane")
            .SetPath("@res://scripts/plane.lua");
    }
    {
        auto cockpit = p_scene->CreateMeshEntity("cockpit",
                                                 material_red,
                                                 MakeCubeMesh(COCKPIT_POINTS));
        p_scene->AttachChild(cockpit, plane);
    }
    {
        auto engine = p_scene->CreateCubeEntity("engine",
                                                material_white,
                                                Vector3f(1.0f, 2.5f, 2.5f),
                                                math::Translate(Vector3f(5, 0, 0)));
        p_scene->AttachChild(engine, plane);
    }
    {
        auto side_wing = p_scene->CreateCubeEntity("side_wing",
                                                   material_red,
                                                   Vector3f(1.5f, 0.25f, 6.0f),
                                                   math::Translate(Vector3f(0.0f, 1.5f, 0.0f)));
        p_scene->AttachChild(side_wing, plane);
    }
    {
        auto tail = p_scene->CreateCubeEntity("tail",
                                              material_red,
                                              Vector3f(0.75f, 1.0f, 0.25f),
                                              math::Translate(Vector3f(-4, 2, 0)));
        p_scene->AttachChild(tail, plane);
    }
    {
        auto wind_shield = p_scene->CreateCubeEntity("wind_shield",
                                                     material_white_transparent,
                                                     Vector3f(0.15f, 0.75f, 1.0f),
                                                     math::Translate(Vector3f(1.8f, 2.7f, 0.0f)));
        ObjectComponent* obj = p_scene->GetComponent<ObjectComponent>(wind_shield);
        obj->flags |= ObjectComponent::FLAG_TRANSPARENT;
        p_scene->AttachChild(wind_shield, plane);
    }
    {
        auto wheel_protection_1 = p_scene->CreateCubeEntity("wheel_protection_1",
                                                            material_red,
                                                            Vector3f(1.5f, 0.75f, 0.5f),
                                                            math::Translate(Vector3f(2.5f, -2.0f, 2.5f)));
        p_scene->AttachChild(wheel_protection_1, plane);
    }
    {
        auto wheel_protection_2 = p_scene->CreateCubeEntity("wheel_protection_2",
                                                            material_red,
                                                            Vector3f(1.5f, 0.75f, 0.5f),
                                                            math::Translate(Vector3f(2.5f, -2.0f, -2.5f)));
        p_scene->AttachChild(wheel_protection_2, plane);
    }
    {
        auto wheel_axis = p_scene->CreateCubeEntity("wheel_axis",
                                                    material_brown,
                                                    Vector3f(0.5f, 0.5f, 2.95f),
                                                    math::Translate(Vector3f(2.5f, -2.8f, -0.0f)));
        p_scene->AttachChild(wheel_axis, plane);
    }

    {
        Matrix4x4f rotation = math::Rotate(Degree(160.f), Vector3f(0, 0, 1));
        Matrix4x4f translation = math::Translate(Vector3f(-3.3f, -0.2f, 0.0f));
        auto suspension = p_scene->CreateCubeEntity("suspension",
                                                    material_red,
                                                    Vector3f(0.2f, 1.0f, 0.2f),
                                                    translation * rotation);
        p_scene->AttachChild(suspension, plane);
    }
    {
        auto tire_1 = p_scene->CreateCubeEntity("tire_1",
                                                material_dark_brown,
                                                Vector3f(1.2f, 1.2f, 0.2f),
                                                math::Translate(Vector3f(2.5f, -2.8f, 2.5f)));
        p_scene->AttachChild(tire_1, plane);
        auto tire_2 = p_scene->CreateCubeEntity("tire_2",
                                                material_dark_brown,
                                                Vector3f(1.2f, 1.2f, 0.2f),
                                                math::Translate(Vector3f(2.5f, -2.8f, -2.5f)));
        p_scene->AttachChild(tire_2, plane);
        auto tire_3 = p_scene->CreateCubeEntity("tire_3",
                                                material_brown,
                                                Vector3f(0.4f, 0.4f, 0.15f),
                                                math::Translate(Vector3f(-3.5f, -0.8f, 0.0f)));
        p_scene->AttachChild(tire_3, plane);
        auto tire_4 = p_scene->CreateCubeEntity("tire_4",
                                                material_dark_brown,
                                                Vector3f(0.6f, 0.6f, 0.1f),
                                                math::Translate(Vector3f(-3.5f, -0.8f, 0.0f)));
        p_scene->AttachChild(tire_4, plane);
        auto body = p_scene->CreateCubeEntity("body",
                                              material_brown,
                                              Vector3f(1.5f) * 0.7f,
                                              math::Translate(Vector3f(.2f, 1.5f, 0.0f)));
        p_scene->AttachChild(body, plane);
        auto face = p_scene->CreateCubeEntity("face",
                                              material_pink,
                                              Vector3f(1.0f) * 0.7f,
                                              math::Translate(Vector3f(.0f, 2.7f, 0.0f)));

        p_scene->AttachChild(face, plane);
        auto hair_side = p_scene->CreateCubeEntity("hair_side",
                                                   material_dark_brown,
                                                   Vector3f(1.2f, 0.6f, 1.2f) * 0.7f,
                                                   math::Translate(Vector3f(-.3f, 3.2f, 0.0f)));
        p_scene->AttachChild(hair_side, plane);

        for (int i = 0; i < 12; ++i) {
            const int col = i % 3;
            const int row = i / 3;
            Vector3f translation(-0.9f + row * 0.4f, 3.5f, -0.4f + col * 0.4f);
            float scale_y = col == 1 ? 0.7f : 0.6f;
            auto hair = p_scene->CreateCubeEntity(std::format("hair_{}", i), material_dark_brown, Vector3f(0.27f, scale_y, 0.27f), math::Translate(translation));
            TransformComponent* transform = p_scene->GetComponent<TransformComponent>(hair);
            float s = 0.5f + (3 - row) * 0.15f;
            transform->SetScale(Vector3f(1, s, 1));
            p_scene->AttachChild(hair, plane);

            p_scene->Create<LuaScriptComponent>(hair)
                .SetClassName("Hair")
                .SetPath("@res://scripts/hair.lua");
        }
    }

    auto propeller = p_scene->CreateTransformEntity("propeller");
    {
        TransformComponent* transform = p_scene->GetComponent<TransformComponent>(propeller);
        transform->Translate(Vector3f(6.0f, 0.0f, 0.0f));
        p_scene->AttachChild(propeller, plane);
        p_scene->Create<LuaScriptComponent>(propeller)
            .SetClassName("Propeller")
            .SetPath("@res://scripts/propeller.lua");
    }
    {
        auto pivot = p_scene->CreateMeshEntity("pivot",
                                               material_brown,
                                               MakeConeMesh(0.4f, 2.0f, 8));

        TransformComponent* transform = p_scene->GetComponent<TransformComponent>(pivot);
        transform->RotateZ(Degree(90.0f));
        p_scene->AttachChild(pivot, propeller);
    }
    {
        auto blade1 = p_scene->CreateCubeEntity("blade1",
                                                material_dark_brown,
                                                Vector3f(0.2f, 4.0f, 0.2f),
                                                math::Translate(Vector3f(0.4f, 0.0f, 0.0f)));
        p_scene->AttachChild(blade1, propeller);
    }
    {
        auto blade2 = p_scene->CreateCubeEntity("blade2",
                                                material_dark_brown,
                                                Vector3f(0.2f, 0.2f, 4.0f),
                                                math::Translate(Vector3f(0.4f, 0.0f, 0.0f)));
        p_scene->AttachChild(blade2, propeller);
    }
}

Scene* CreateTheAviatorScene() {
    SceneCreator creator;
    return creator.CreateScene();
}

}  // namespace my
