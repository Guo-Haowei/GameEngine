#include "engine/core/base/random.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/math/color.h"
#include "engine/core/math/geometry.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/scene.h"
#include "engine/scene/scriptable_entity.h"

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

static constexpr float OCEAN_RADIUS = 240.0f;
static constexpr int CLOUD_COUNT = 20;

// @TODO:
// * cascaded shadow map
// * alpha blending
// * collision
// * fog
// * dynamic buffer
// * 2D UI

static MeshComponent MakeOceanMesh(float p_radius,
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
            float x_1 = p_radius * glm::cos(angle_1);
            float z_1 = p_radius * glm::sin(angle_1);
            float angle_2 = 2.0f * pi * ((index + 1) == p_sectors ? 0 : (index + 1)) / p_sectors;
            float x_2 = p_radius * glm::cos(angle_2);
            float z_2 = p_radius * glm::sin(angle_2);

            Vector3f point_1(x_1, y, z_1);
            Vector3f point_2(x_1, y + height_step, z_1);

            Vector3f point_3(x_2, y, z_2);
            Vector3f point_4(x_2, y + height_step, z_2);

            Vector3f AB = point_1 - point_2;
            Vector3f AC = point_1 - point_3;
            Vector3f normal = glm::normalize(glm::cross(AB, AC));

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

Scene* CreateTheAviatorScene() {
    const std::array<Vector3f, 8> cockpit_points = {
        Vector3f(-4.f, +1.5f, +0.5f),  // A
        Vector3f(-4.f, +0.5f, +0.5f),  // B
        Vector3f(+4.f, -2.5f, +2.5f),  // C
        Vector3f(+4.f, +2.5f, +2.5f),  // D
        Vector3f(-4.f, +1.5f, -0.5f),  // E
        Vector3f(-4.f, +0.5f, -0.5f),  // F
        Vector3f(+4.f, -2.5f, -2.5f),  // G
        Vector3f(+4.f, +2.5f, -2.5f),  // H
    };

    const float plane_height = 30.0f;

    ecs::Entity::SetSeed();

    Scene* scene = new Scene;

    auto root = scene->CreateTransformEntity("root");
    scene->m_root = root;

    Vector2i frame_size = DVAR_GET_IVEC2(resolution);
    // editor camera
    {
        auto editor_camera = scene->CreatePerspectiveCameraEntity("editor_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(editor_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0, plane_height, 80));
        camera->SetEditorCamera();
        scene->AttachChild(editor_camera, root);
    }
    // main camera
    {
        auto main_camera = scene->CreatePerspectiveCameraEntity("main_camera", frame_size.x, frame_size.y);
        scene->AttachChild(main_camera, root);

        auto camera = scene->GetComponent<PerspectiveCameraComponent>(main_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0, plane_height, 100));
        camera->SetPrimary();

        class CameraController : public ScriptableEntity {
        protected:
            void OnUpdate(float p_timestep) override {
                PerspectiveCameraComponent* camera = GetComponent<PerspectiveCameraComponent>();
                if (camera) {
                    const Vector2f mouse_move = InputManager::GetSingleton().MouseMove();
                    if (glm::abs(mouse_move.x) > 2.0f) {
                        float angle = camera->GetFovy().GetDegree();
                        angle += mouse_move.x * p_timestep * 2.0f;
                        angle = glm::clamp(angle, 35.0f, 70.0f);
                        camera->SetFovy(Degree(angle));
                    }
                }
            }
        };

        scene->Create<NativeScriptComponent>(main_camera).Bind<CameraController>();
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
    }

#pragma region SETUP_MATERIALS
    // colors
    const Color red = Color::Hex(0xCE190A);
    const Color white = Color::Hex(0XD8D0D1);
    const Color dark_brown = Color::Hex(0x23190F);
    const Color brown = Color::Hex(0x59332E);
    const Color blue = Color::Hex(0X10A8A3);
    // Color pink = Color::Hex(0xF5986E);

    constexpr float default_roughness = 0.8f;
    constexpr float default_metallic = 0.2f;
    ecs::Entity material_red = scene->CreateMaterialEntity("material_red");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_red);
        material->baseColor = red.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_white = scene->CreateMaterialEntity("material_white");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_white);
        material->baseColor = white.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_dark_brown = scene->CreateMaterialEntity("material_dark_brown");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_dark_brown);
        material->baseColor = dark_brown.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_brown = scene->CreateMaterialEntity("material_brown");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_brown);
        material->baseColor = brown.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_blue = scene->CreateMaterialEntity("material_blue");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_blue);
        material->baseColor = blue.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
#pragma endregion SETUP_MATERIALS

#pragma region SETUP_PLANE
    // create plane
    auto plane = scene->CreateTransformEntity("plane");
    {
        TransformComponent* transform = scene->GetComponent<TransformComponent>(plane);
        transform->Translate(Vector3f(0.0f, plane_height, 0.0f));

        scene->AttachChild(plane, root);
        auto& script = scene->Create<LuaScriptComponent>(plane);
        script.SetScript("@res://scripts/plane.lua");
    }
    {
        auto cockpit = scene->CreateMeshEntity("cockpit",
                                               material_red,
                                               MakeCubeMesh(cockpit_points));
        scene->AttachChild(cockpit, plane);
    }
    {
        auto engine = scene->CreateCubeEntity("engine",
                                              material_white,
                                              Vector3f(1.0f, 2.5f, 2.5f),
                                              glm::translate(Vector3f(5, 0, 0)));
        scene->AttachChild(engine, plane);
    }
    {
        auto side_wing = scene->CreateCubeEntity("side_wing",
                                                 material_red,
                                                 Vector3f(1.5f, 0.25f, 6.0f),
                                                 glm::translate(Vector3f(0.0f, 1.5f, 0.0f)));
        scene->AttachChild(side_wing, plane);
    }
    {
        auto tail = scene->CreateCubeEntity("tail",
                                            material_red,
                                            Vector3f(0.75f, 1.0f, 0.25f),
                                            glm::translate(Vector3f(-4, 2, 0)));
        scene->AttachChild(tail, plane);
    }
    {
        auto wind_shield = scene->CreateCubeEntity("wind_shield",
                                                   material_white,
                                                   Vector3f(0.15f, 0.75f, 1.0f),
                                                   glm::translate(Vector3f(0.5f, 2.7f, 0.0f)));
        scene->AttachChild(wind_shield, plane);
    }
    {
        auto wheel_protection_1 = scene->CreateCubeEntity("wheel_protection_1",
                                                          material_red,
                                                          Vector3f(1.5f, 0.75f, 0.5f),
                                                          glm::translate(Vector3f(2.5f, -2.0f, 2.5f)));
        scene->AttachChild(wheel_protection_1, plane);
    }
    {
        auto wheel_protection_2 = scene->CreateCubeEntity("wheel_protection_2",
                                                          material_red,
                                                          Vector3f(1.5f, 0.75f, 0.5f),
                                                          glm::translate(Vector3f(2.5f, -2.0f, -2.5f)));
        scene->AttachChild(wheel_protection_2, plane);
    }
    {
        auto tire_1 = scene->CreateCubeEntity("tire_1",
                                              material_dark_brown,
                                              Vector3f(1.2f, 1.2f, 0.2f),
                                              glm::translate(Vector3f(2.5f, -2.8f, 2.5f)));
        scene->AttachChild(tire_1, plane);
    }
    {
        auto tire_2 = scene->CreateCubeEntity("tire_2",
                                              material_dark_brown,
                                              Vector3f(1.2f, 1.2f, 0.2f),
                                              glm::translate(Vector3f(2.5f, -2.8f, -2.5f)));
        scene->AttachChild(tire_2, plane);
    }
    {
        auto wheel_axis = scene->CreateCubeEntity("wheel_axis",
                                                  material_brown,
                                                  Vector3f(0.5f, 0.5f, 2.95f),
                                                  glm::translate(Vector3f(2.5f, -2.8f, -0.0f)));
        scene->AttachChild(wheel_axis, plane);
    }

    {
        Matrix4x4f rotation = glm::rotate(Degree(160.f).GetRadians(), Vector3f(0, 0, 1));
        Matrix4x4f translation = glm::translate(Vector3f(-3.3f, -0.2f, 0.0f));
        auto suspension = scene->CreateCubeEntity("suspension",
                                                  material_red,
                                                  Vector3f(0.2f, 1.0f, 0.2f),
                                                  translation * rotation);
        scene->AttachChild(suspension, plane);
    }
    {
        auto tire_3 = scene->CreateCubeEntity("tire_3",
                                              material_brown,
                                              Vector3f(0.4f, 0.4f, 0.15f),
                                              glm::translate(Vector3f(-3.5f, -0.8f, 0.0f)));
        scene->AttachChild(tire_3, plane);
    }
    {
        auto tire_4 = scene->CreateCubeEntity("tire_4",
                                              material_dark_brown,
                                              Vector3f(0.6f, 0.6f, 0.1f),
                                              glm::translate(Vector3f(-3.5f, -0.8f, 0.0f)));
        scene->AttachChild(tire_4, plane);
    }

    auto propeller = scene->CreateTransformEntity("propeller");
    {
        TransformComponent* transform = scene->GetComponent<TransformComponent>(propeller);
        transform->Translate(Vector3f(6.0f, 0.0f, 0.0f));
        scene->AttachChild(propeller, plane);
        auto& script = scene->Create<LuaScriptComponent>(propeller);
        script.SetScript("@res://scripts/propeller.lua");
    }
    {
        auto pivot = scene->CreateMeshEntity("pivot",
                                             material_brown,
                                             MakeConeMesh(0.4f, 2.0f, 8));
        TransformComponent* transform = scene->GetComponent<TransformComponent>(pivot);
        transform->RotateZ(Degree(90.0f));
        scene->AttachChild(pivot, propeller);
    }
    {
        auto blade1 = scene->CreateCubeEntity("blade1",
                                              material_dark_brown,
                                              Vector3f(0.2f, 4.0f, 0.2f),
                                              glm::translate(Vector3f(0.4f, 0.0f, 0.0f)));
        scene->AttachChild(blade1, propeller);
    }
    {
        auto blade2 = scene->CreateCubeEntity("blade2",
                                              material_dark_brown,
                                              Vector3f(0.2f, 0.2f, 4.0f),
                                              glm::translate(Vector3f(0.4f, 0.0f, 0.0f)));
        scene->AttachChild(blade2, propeller);
    }

    auto world = scene->CreateTransformEntity("world");
    {
        scene->AttachChild(world, root);
        auto transform = scene->GetComponent<TransformComponent>(world);
        transform->Translate(Vector3f(0.0f, -OCEAN_RADIUS, 0.0f));
    }

    auto earth = scene->CreateTransformEntity("earth");
    {
        auto& script = scene->Create<LuaScriptComponent>(world);
        script.SetScript("@res://scripts/world.lua");
        scene->AttachChild(earth, world);
    }

    // ocean
    {
        auto ocean = scene->CreateMeshEntity("ocean", material_blue, MakeOceanMesh(OCEAN_RADIUS, 320.0f, 60, 16));
        ObjectComponent* object = scene->GetComponent<ObjectComponent>(ocean);
        DEV_ASSERT(object);

        MeshComponent* mesh = scene->GetComponent<MeshComponent>(object->meshId);
        DEV_ASSERT(mesh);
        mesh->flags |= MeshComponent::DYNAMIC;

        auto transform = scene->GetComponent<TransformComponent>(ocean);
        DEV_ASSERT(transform);
        transform->RotateX(Degree(90.0f));
        transform->RotateZ(Degree(90.0f));
        scene->AttachChild(ocean, earth);

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
                    Vector3f normal = glm::normalize(glm::cross(AB, AC));
                    normals.emplace_back(normal);
                    normals.emplace_back(normal);
                    normals.emplace_back(normal);
                }
            }

            std::vector<Wave> m_waves;
        };
        scene->Create<NativeScriptComponent>(ocean).Bind<OceanScript>();
    }
#pragma endregion SETUP_PLANE

#pragma region SETUP_SKY
    constexpr float TWO_PI = 2.0f * glm::pi<float>();
    auto create_cloud = [&](int p_index, ecs::Entity p_parent) {
        int block = 3 + static_cast<int>(glm::floor(Random::Float() * 3));
        for (int block_index = 0; block_index < block; ++block_index) {
            std::string name = std::format("block_{}_{}", p_index, block_index);
            const float scale = 0.6f + Random::Float() * 0.4f;
            auto cloud = scene->CreateCubeEntity(name, material_white, Vector3f(5.0f * scale));
            auto transform = scene->GetComponent<TransformComponent>(cloud);
            Vector3f position(block_index * 6.0f, Random::Float() * 6.0f, Random::Float() * 6.0f);
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
        Matrix4x4f matrix = glm::translate(Vector3f(x, y, -50.0f)) * glm::rotate(angle, Vector3f(0, 0, -1));
        transform->SetLocalTransform(matrix);
        create_cloud(cloud_index, cloud);
    }
#pragma endregion SETUP_SKY

    {
        auto sky_light = scene->CreateHemisphereLightEntity("sky_light", "@res://images/ibl/sky.hdr");
        scene->AttachChild(sky_light, root);
    }

    return scene;
}

}  // namespace my
