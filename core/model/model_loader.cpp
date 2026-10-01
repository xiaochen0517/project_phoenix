#include "model/model_loader.h"

#include "log/app_log.h"
#include "raylib.h"
#include "raymath.h"

#include <cmath>

namespace model_loader {
namespace {
// 无动画时使用的哨兵值
constexpr int kNoAnimation = -1;

// glTF 导入时 raylib 将关键帧时间戳换算为 60fps 帧索引, 播放基准取 60fps
constexpr float kAnimationFps = 60.0f;

bool validAnimation(const ModelAnimation& animation) {
    return animation.keyframeCount > 0 && animation.keyframePoses != nullptr;
}
} // namespace

ModelScene::~ModelScene() {
    unload();
}

bool ModelScene::load(const std::string& path, std::string* error) {
    unload();

    Model model = LoadModel(path.c_str());
    // raylib 加载失败时返回空 Model (meshCount == 0), 并通过 TraceLog 输出具体原因
    if (model.meshCount == 0 || model.meshes == nullptr) {
        if (error != nullptr) {
            *error = "raylib LoadModel 失败 (文件不存在或格式不支持): " + path;
        }
        UnloadModel(model);
        app_log::error("Model load failed: " + path);
        return false;
    }

    int animCount = 0;
    ModelAnimation* animations = LoadModelAnimations(path.c_str(), &animCount);
    // glTF 无动画时返回 nullptr / 0, 属正常情况; 其它格式可能输出警告日志

    model_ = new Model(model);
    animations_ = animations;
    animationCount_ = animCount;
    path_ = path;
    loaded_ = true;

    animationNames_.clear();
    for (int i = 0; i < animCount; ++i) {
        animationNames_.emplace_back(animations[i].name[0] != '\0' ? animations[i].name
                                                                   : "animation_" + std::to_string(i));
    }
    activeAnimation_ = animCount > 0 ? 0 : kNoAnimation;
    playing_ = true;
    loop_ = true;
    speed_ = 1.0f;
    currentFrame_ = 0.0f;

    const ModelStats s = stats();
    app_log::info("Model loaded: " + path + " (meshes=" + std::to_string(s.meshCount) +
                  ", vertices=" + std::to_string(s.vertexCount) + ", triangles=" + std::to_string(s.triangleCount) +
                  ", materials=" + std::to_string(s.materialCount) +
                  ", animations=" + std::to_string(s.animationCount) + ")");
    return true;
}

void ModelScene::unload() {
    if (!loaded_) {
        return;
    }

    if (animations_ != nullptr) {
        UnloadModelAnimations(static_cast<ModelAnimation*>(animations_), animationCount_);
        animations_ = nullptr;
    }
    if (model_ != nullptr) {
        UnloadModel(*static_cast<Model*>(model_));
        delete static_cast<Model*>(model_);
        model_ = nullptr;
    }

    animationCount_ = 0;
    animationNames_.clear();
    activeAnimation_ = kNoAnimation;
    currentFrame_ = 0.0f;
    path_.clear();
    loaded_ = false;
}

void ModelScene::setActiveAnimation(int index) {
    if (index < 0 || index >= animationCount_) {
        activeAnimation_ = kNoAnimation;
        return;
    }
    activeAnimation_ = index;
    currentFrame_ = 0.0f;
}

void ModelScene::setSpeed(float speed) {
    speed_ = speed < 0.0f ? 0.0f : speed;
}

void ModelScene::update(float deltaTime) {
    if (!loaded_ || activeAnimation_ == kNoAnimation || !playing_) {
        return;
    }

    Model* model = static_cast<Model*>(model_);
    ModelAnimation* animations = static_cast<ModelAnimation*>(animations_);
    const ModelAnimation& animation = animations[activeAnimation_];
    if (!validAnimation(animation)) {
        return;
    }

    const float frameCount = static_cast<float>(animation.keyframeCount);
    currentFrame_ += deltaTime * kAnimationFps * speed_;

    if (loop_) {
        currentFrame_ = std::fmod(currentFrame_, frameCount);
        if (currentFrame_ < 0.0f) {
            currentFrame_ += frameCount;
        }
    } else if (currentFrame_ >= frameCount) {
        // 播完最后一帧后停止
        currentFrame_ = frameCount - 1.0f;
        playing_ = false;
    }

    UpdateModelAnimation(*model, animation, static_cast<int>(currentFrame_) % animation.keyframeCount);
}

void ModelScene::draw(const Transform& transform, bool drawWires) const {
    if (!loaded_) {
        return;
    }

    const Model* model = static_cast<const Model*>(model_);
    const Vector3 position{transform.position[0], transform.position[1], transform.position[2]};
    Vector3 axis{transform.rotationAxis[0], transform.rotationAxis[1], transform.rotationAxis[2]};
    if (Vector3LengthSqr(axis) == 0.0f) {
        axis = Vector3{1.0f, 0.0f, 0.0f};
    }
    const Vector3 scale{transform.scale[0], transform.scale[1], transform.scale[2]};

    if (drawWires) {
        DrawModelWiresEx(*model, position, axis, transform.rotationAngle, scale, WHITE);
    } else {
        DrawModelEx(*model, position, axis, transform.rotationAngle, scale, WHITE);
    }
}

void ModelScene::drawBounds(const Transform& transform) const {
    if (!loaded_) {
        return;
    }

    const Model* model = static_cast<const Model*>(model_);
    BoundingBox box = GetModelBoundingBox(*model);
    const Vector3 offset{transform.position[0], transform.position[1], transform.position[2]};
    box.min = Vector3Add(box.min, offset);
    box.max = Vector3Add(box.max, offset);
    DrawBoundingBox(box, LIME);
}

bool ModelScene::bounds(float center[3], float size[3]) const {
    if (!loaded_) {
        return false;
    }

    const Model* model = static_cast<const Model*>(model_);
    const BoundingBox box = GetModelBoundingBox(*model);
    const Vector3 c = Vector3Scale(Vector3Add(box.min, box.max), 0.5f);
    const Vector3 s = Vector3Subtract(box.max, box.min);

    center[0] = c.x;
    center[1] = c.y;
    center[2] = c.z;
    size[0] = s.x;
    size[1] = s.y;
    size[2] = s.z;
    return true;
}

ModelStats ModelScene::stats() const {
    ModelStats result;
    result.animationCount = animationCount_;

    if (loaded_) {
        const Model* model = static_cast<const Model*>(model_);
        result.meshCount = model->meshCount;
        result.materialCount = model->materialCount;
        for (int i = 0; i < model->meshCount; ++i) {
            result.vertexCount += model->meshes[i].vertexCount;
            result.triangleCount += model->meshes[i].triangleCount;
        }
    }
    return result;
}

bool ModelScene::applyTexture(MaterialMapType mapType, const std::string& path, std::string* error) {
    if (!loaded_) {
        if (error != nullptr) {
            *error = "未加载模型, 无法应用贴图";
        }
        return false;
    }

    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        if (error != nullptr) {
            *error = "raylib LoadTexture 失败 (文件不存在或格式不支持): " + path;
        }
        app_log::error("Texture load failed: " + path);
        return false;
    }

    Model* model = static_cast<Model*>(model_);
    const int map = static_cast<int>(mapType);
    for (int i = 0; i < model->materialCount; ++i) {
        // 注意: 仅覆盖引用, 不主动 Unload 旧贴图; 同一贴图可能被多个材质共享, 重复 Unload 会崩溃。
        // 旧贴图随 UnloadModel 一并释放 (加载新模型时会 unload)。
        SetMaterialTexture(&model->materials[i], map, texture);
    }

    app_log::info("Texture applied: " + path + " (map=" + std::to_string(map) +
                  ", materials=" + std::to_string(model->materialCount) + ")");
    return true;
}
} // namespace model_loader
