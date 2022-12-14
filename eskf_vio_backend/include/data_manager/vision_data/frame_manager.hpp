#pragma once
/* 外部依赖 */
/* 内部依赖 */
#include <feature_manager.hpp>

#define FIRST_FRAME_ID 1

namespace ESKF_VIO_BACKEND {
    /* 管理一个关键帧 */
    class Frame {
    public:
        // 此帧所有 camera 观测到的所有特征点，保存为 <feature ID, ptr of feature>
        std::unordered_map<uint32_t, std::shared_ptr<Feature>> features;
        // 此帧对应的 IMU 系的位姿和速度
        Quaternion q_wb = Quaternion::Identity();
        Vector3 p_wb = Vector3::Zero();
        Vector3 v_wb = Vector3::Zero();
        // 此帧的全局索引
        uint32_t id = 0;
        // 此帧对应的时间戳
        double timeStamp = 0.0;
    public:
        /* 构造函数与析构函数 */
        explicit Frame(const uint32_t id, const double timeStamp) :
            id(id), timeStamp(timeStamp) {}
        explicit Frame(const double timeStamp) :
            timeStamp(timeStamp) {}
        virtual ~Frame() = default;
    public:
        /* 初始化关键帧 */
        bool Initialize(const uint32_t id, const double timeStamp);
        /* 为此帧添加一个特征点关联 */
        bool AddFeature(const std::shared_ptr<Feature> &newFeature);
        /* 为此帧添加一组特征点关联 */
        bool AddFeatures(const std::vector<std::shared_ptr<Feature>> &newFeatures);
        /* 提取当前帧与某一帧之间的共视特征点 */
        std::vector<std::shared_ptr<Feature>> GetCovisibleFeatures(const std::shared_ptr<Frame> &target);
        /* 打印出此帧所有信息 */
        void Information(void);
    };


    /* 管理一组相机与 IMU 的外参 */
    class Extrinsic {
    public:
        // 相机与 IMU 之间的外参
        Quaternion q_bc;
        Vector3 p_bc;
    public:
        /* 构造函数与析构函数 */
        Extrinsic() = default;
        virtual ~Extrinsic() = default;
        Extrinsic(const Quaternion &q_bc, const Vector3 &p_bc) :
            q_bc(q_bc), p_bc(p_bc) {}
    };


    /* 管理滑动窗口内所有关键帧 */
    class FrameManager {
    public:
        // 滑动窗口内所有关键帧
        std::deque<std::shared_ptr<Frame>> frames;
    public:
        // 相机与 IMU 之间的外参
        std::vector<Extrinsic> extrinsics;
        // 滑动窗口的大小
        uint32_t maxWindowSize = 5;
        // 关键帧判断相关阈值
        uint32_t minKeyframeTrackedFeatureNum = 25;
        Scalar minKeyframeMeanParallax = 0.2f;
        Scalar maxKeyframeTranslation = 0.3f;
    public:
        /* 构造函数与析构函数 */
        FrameManager() = default;
        virtual ~FrameManager() = default;
    public:
        /* 帧管理器初始化 */
        bool Initialize(const uint32_t maxWindowSize);
        /* 增加新一帧，检查滑窗内 ID 的合法性，并返回新加入帧的 ID */
        bool AddNewFrame(const std::shared_ptr<Frame> &newFrame);
        /* 移除一帧，给定待移动帧在滑动窗口内的索引 (0 -> max window size - 1) */
        bool RemoveFrame(const uint32_t localIndex);
        /* 提取某一个关键帧的指针 */
        std::shared_ptr<Frame> GetFrame(const uint32_t frameID);
        /* 获取滑动窗口内最大的 frame ID */
        uint32_t GetMaxFrameID(void);
        /* 判断滑动窗口是否需要进行边缘化操作 */
        bool NeedMarginalize(void);
        /* 判断是否关键帧 */
        bool IsKeyFrame(const std::shared_ptr<Frame> &frame);
        /* 打印出滑动窗口内所有关键帧的信息 */
        void Information(void);
    };
}