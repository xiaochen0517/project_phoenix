#pragma once

#include <string>
#include <vector>

namespace model_loader {
    // 模型变换: 位置 / 旋转 (轴 + 角度) / 缩放
    struct Transform {
        float position[3] = {0.0f, 0.0f, 0.0f};
        float rotationAxis[3] = {1.0f, 0.0f, 0.0f}; // 旋转轴 (需非零向量)
        float rotationAngle = 0.0f; // 旋转角度 (度)
        float scale[3] = {1.0f, 1.0f, 1.0f};
    };

    // 模型统计信息
    struct ModelStats {
        int meshCount = 0;
        int materialCount = 0;
        int vertexCount = 0;
        int triangleCount = 0;
        int animationCount = 0;
    };

    // 材质贴图槽 (取值与 raylib MaterialMapIndex 一致, 避免在头文件暴露 raylib 类型)
    enum class MaterialMapType {
        Albedo = 0, // 基础色 / 漫反射
        Metalness = 1, // 金属度
        Normal = 2, // 法线
        Roughness = 3, // 粗糙度
        Occlusion = 4, // 环境光遮蔽
        Emission = 5, // 自发光
    };

    // 模型加载与动画播放封装 (内部持有 raylib Model / ModelAnimation, 不向应用层暴露)
    class ModelScene {
    public:
        ModelScene() = default;

        ~ModelScene();

        ModelScene(const ModelScene &) = delete;

        ModelScene &operator=(const ModelScene &) = delete;

        // 从指定路径加载模型 (glTF/GLB 等 raylib 支持的格式); 失败返回 false 并填充 error
        bool load(const std::string &path, std::string *error = nullptr);

        void unload();

        bool isLoaded() const { return loaded_; }
        const std::string &path() const { return path_; }

        // 动画: index 取 [0, animationCount), -1 表示不播放动画
        int animationCount() const { return animationCount_; }
        const std::vector<std::string> &animationNames() const { return animationNames_; }

        void setActiveAnimation(int index);

        int activeAnimation() const { return activeAnimation_; }
        void setPlaying(bool playing) { playing_ = playing; }
        bool playing() const { return playing_; }
        void setLoop(bool loop) { loop_ = loop; }
        bool loop() const { return loop_; }

        void setSpeed(float speed);

        float speed() const { return speed_; }
        float currentFrame() const { return currentFrame_; }

        // 每帧调用: 推进动画时间 (60fps 基准)
        void update(float deltaTime);

        // 绘制模型; drawWires 为 true 时绘制线框
        void draw(const Transform &transform, bool drawWires) const;

        // 绘制模型包围盒 (仅应用平移, 不应用旋转)
        void drawBounds(const Transform &transform) const;

        // 模型包围盒中心与尺寸 (未变换的本地空间), 供相机「适配模型」使用
        bool bounds(float center[3], float size[3]) const;

        ModelStats stats() const;

        // 动态加载贴图并绑定到模型所有材质的指定贴图槽 (mapType 见 MaterialMapType)
        bool applyTexture(MaterialMapType mapType, const std::string &path, std::string *error = nullptr);

    private:
        bool loaded_ = false;
        std::string path_;

        void *model_ = nullptr; // raylib Model*
        void *animations_ = nullptr; // raylib ModelAnimation*
        int animationCount_ = 0;

        std::vector<std::string> animationNames_;
        int activeAnimation_ = -1; // -1 = 不播放
        bool playing_ = true;
        bool loop_ = true;
        float speed_ = 1.0f;
        float currentFrame_ = 0.0f;
    };
} // namespace model_loader
