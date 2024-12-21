//  @TODO: refactor
#include "engine/scene/camera_controller.h"
#include "the_aviator_script.h"

// @TODO: remove
#include "engine/math/matrix_transform.h"
#include "engine/scene/scene_serialization.h"

namespace my {

// @TODO:
// * cascaded shadow map
// * fog
// * 2D UI
// * draw elements and draw array
// * motion blur

using math::AABB;

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
        camera->SetPosition(Vector3f(0.0f, plane_height + 10.0f, 80.0f));
        camera->SetPrimary();
        class InGameDebugCameraController : public EditorCameraController {
        public:
            InGameDebugCameraController() {
                SetScrollSpeed(10.0f);
                SetMoveSpeed(30.0f);
            }
        };
#if 0
        scene->Create<NativeScriptComponent>(main_camera).Bind<InGameDebugCameraController>();
#else
        scene->Create<NativeScriptComponent>(main_camera).Bind<CameraScript>();
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

#pragma region SETUP_MATERIALS

    constexpr float default_roughness = 0.8f;
    constexpr float default_metallic = 0.2f;
    auto material_red = scene->CreateMaterialEntity("material_red");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_red);
        material->baseColor = RED_COLOR.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    auto material_white = scene->CreateMaterialEntity("material_white");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_white);
        material->baseColor = WHITE_COLOR.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    auto material_white_transparent = scene->CreateMaterialEntity("material_white_transparent");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_white_transparent);
        material->baseColor = WHITE_COLOR.ToVector4f();
        material->baseColor.a = 0.5f;
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    auto material_dark_brown = scene->CreateMaterialEntity("material_dark_brown");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_dark_brown);
        material->baseColor = DRAK_BROWN_COLOR.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    auto material_brown = scene->CreateMaterialEntity("material_brown");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_brown);
        material->baseColor = BROWN_COLOR.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    auto material_blue_transparent = scene->CreateMaterialEntity("material_blue_transparent");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_blue_transparent);
        material->baseColor = BLUE_COLOR.ToVector4f();
        material->baseColor.a = 0.8f;
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    auto material_pink = scene->CreateMaterialEntity("material_pink");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_pink);
        material->baseColor = PINK_COLOR.ToVector4f();
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

        // @TODO: fix collision boundary, probably want to visualize it
        auto& rigid_body = scene->Create<RigidBodyComponent>(plane)
                               .InitSphere(4.0f);

        rigid_body.collisionType = COLLISION_BIT_PLAYER;
        rigid_body.collisionMask = COLLISION_BIT_BATTERY | COLLISION_BIT_ROCK;

        scene->AttachChild(plane, root);
        scene->Create<NativeScriptComponent>(plane).Bind<PlaneScript>();
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
                                              math::Translate(Vector3f(5, 0, 0)));
        scene->AttachChild(engine, plane);
    }
    {
        auto side_wing = scene->CreateCubeEntity("side_wing",
                                                 material_red,
                                                 Vector3f(1.5f, 0.25f, 6.0f),
                                                 math::Translate(Vector3f(0.0f, 1.5f, 0.0f)));
        scene->AttachChild(side_wing, plane);
    }
    {
        auto tail = scene->CreateCubeEntity("tail",
                                            material_red,
                                            Vector3f(0.75f, 1.0f, 0.25f),
                                            math::Translate(Vector3f(-4, 2, 0)));
        scene->AttachChild(tail, plane);
    }
    {
        auto wind_shield = scene->CreateCubeEntity("wind_shield",
                                                   material_white_transparent,
                                                   Vector3f(0.15f, 0.75f, 1.0f),
                                                   math::Translate(Vector3f(1.8f, 2.7f, 0.0f)));
        ObjectComponent* obj = scene->GetComponent<ObjectComponent>(wind_shield);
        obj->flags |= ObjectComponent::FLAG_TRANSPARENT;
        scene->AttachChild(wind_shield, plane);
    }
    {
        auto wheel_protection_1 = scene->CreateCubeEntity("wheel_protection_1",
                                                          material_red,
                                                          Vector3f(1.5f, 0.75f, 0.5f),
                                                          math::Translate(Vector3f(2.5f, -2.0f, 2.5f)));
        scene->AttachChild(wheel_protection_1, plane);
    }
    {
        auto wheel_protection_2 = scene->CreateCubeEntity("wheel_protection_2",
                                                          material_red,
                                                          Vector3f(1.5f, 0.75f, 0.5f),
                                                          math::Translate(Vector3f(2.5f, -2.0f, -2.5f)));
        scene->AttachChild(wheel_protection_2, plane);
    }
    {
        auto wheel_axis = scene->CreateCubeEntity("wheel_axis",
                                                  material_brown,
                                                  Vector3f(0.5f, 0.5f, 2.95f),
                                                  math::Translate(Vector3f(2.5f, -2.8f, -0.0f)));
        scene->AttachChild(wheel_axis, plane);
    }

    {
        Matrix4x4f rotation = math::Rotate(Degree(160.f), Vector3f(0, 0, 1));
        Matrix4x4f translation = math::Translate(Vector3f(-3.3f, -0.2f, 0.0f));
        auto suspension = scene->CreateCubeEntity("suspension",
                                                  material_red,
                                                  Vector3f(0.2f, 1.0f, 0.2f),
                                                  translation * rotation);
        scene->AttachChild(suspension, plane);
    }
    {
        auto tire_1 = scene->CreateCubeEntity("tire_1",
                                              material_dark_brown,
                                              Vector3f(1.2f, 1.2f, 0.2f),
                                              math::Translate(Vector3f(2.5f, -2.8f, 2.5f)));
        scene->AttachChild(tire_1, plane);
        auto tire_2 = scene->CreateCubeEntity("tire_2",
                                              material_dark_brown,
                                              Vector3f(1.2f, 1.2f, 0.2f),
                                              math::Translate(Vector3f(2.5f, -2.8f, -2.5f)));
        scene->AttachChild(tire_2, plane);
        auto tire_3 = scene->CreateCubeEntity("tire_3",
                                              material_brown,
                                              Vector3f(0.4f, 0.4f, 0.15f),
                                              math::Translate(Vector3f(-3.5f, -0.8f, 0.0f)));
        scene->AttachChild(tire_3, plane);
        auto tire_4 = scene->CreateCubeEntity("tire_4",
                                              material_dark_brown,
                                              Vector3f(0.6f, 0.6f, 0.1f),
                                              math::Translate(Vector3f(-3.5f, -0.8f, 0.0f)));
        scene->AttachChild(tire_4, plane);
        auto body = scene->CreateCubeEntity("body",
                                            material_brown,
                                            Vector3f(1.5f) * 0.7f,
                                            math::Translate(Vector3f(.2f, 1.5f, 0.0f)));
        scene->AttachChild(body, plane);
        auto face = scene->CreateCubeEntity("face",
                                            material_pink,
                                            Vector3f(1.0f) * 0.7f,
                                            math::Translate(Vector3f(.0f, 2.7f, 0.0f)));

        scene->AttachChild(face, plane);
        auto hair_side = scene->CreateCubeEntity("hair_side",
                                                 material_dark_brown,
                                                 Vector3f(1.2f, 0.6f, 1.2f) * 0.7f,
                                                 math::Translate(Vector3f(-.3f, 3.2f, 0.0f)));
        scene->AttachChild(hair_side, plane);

        for (int i = 0; i < 12; ++i) {
            const int col = i % 3;
            const int row = i / 3;
            Vector3f translation(-0.9f + row * 0.4f, 3.5f, -0.4f + col * 0.4f);
            float scale_y = col == 1 ? 0.7f : 0.6f;
            auto hair = scene->CreateCubeEntity(std::format("hair_{}", i), material_dark_brown, Vector3f(0.27f, scale_y, 0.27f), math::Translate(translation));
            TransformComponent* transform = scene->GetComponent<TransformComponent>(hair);
            float s = 0.5f + (3 - row) * 0.15f;
            transform->SetScale(Vector3f(1, s, 1));
            scene->AttachChild(hair, plane);

            scene->Create<NativeScriptComponent>(hair).Bind<HairScript>();
        }
    }

    auto propeller = scene->CreateTransformEntity("propeller");
    {
        TransformComponent* transform = scene->GetComponent<TransformComponent>(propeller);
        transform->Translate(Vector3f(6.0f, 0.0f, 0.0f));
        scene->AttachChild(propeller, plane);
        scene->Create<NativeScriptComponent>(propeller).Bind<PropellerScript>();
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
                                              math::Translate(Vector3f(0.4f, 0.0f, 0.0f)));
        scene->AttachChild(blade1, propeller);
    }
    {
        auto blade2 = scene->CreateCubeEntity("blade2",
                                              material_dark_brown,
                                              Vector3f(0.2f, 0.2f, 4.0f),
                                              math::Translate(Vector3f(0.4f, 0.0f, 0.0f)));
        scene->AttachChild(blade2, propeller);
    }
#pragma endregion SETUP_PLANE

    auto world = scene->CreateTransformEntity("world");
    {
        scene->AttachChild(world, root);
        auto transform = scene->GetComponent<TransformComponent>(world);
        transform->Translate(Vector3f(0.0f, -OCEAN_RADIUS, 0.0f));
    }

    auto earth = scene->CreateTransformEntity("earth");
    {
        scene->AttachChild(earth, world);
        scene->Create<NativeScriptComponent>(earth).Bind<EarthScript>();
    }

#pragma region SETUP_OCEAN
    // ocean
    {
        auto ocean = scene->CreateMeshEntity("ocean", material_blue_transparent, MakeOceanMesh(OCEAN_RADIUS, 320.0f, 60, 16));
        ObjectComponent* object = scene->GetComponent<ObjectComponent>(ocean);
        object->flags |= ObjectComponent::FLAG_TRANSPARENT;

        DEV_ASSERT(object);

        MeshComponent* mesh = scene->GetComponent<MeshComponent>(object->meshId);
        DEV_ASSERT(mesh);
        mesh->flags |= MeshComponent::DYNAMIC;

        auto transform = scene->GetComponent<TransformComponent>(ocean);
        DEV_ASSERT(transform);
        transform->RotateX(Degree(90.0f));
        transform->RotateZ(Degree(90.0f));
        scene->AttachChild(ocean, earth);

        scene->Create<NativeScriptComponent>(ocean).Bind<OceanScript>();
    }
#pragma endregion SETUP_OCEAN

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

}  // namespace my
