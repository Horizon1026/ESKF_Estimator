#pragma once
/* 外部依赖 */
/* 内部依赖 */
#include <typedef.hpp>

/* 后端的数据加载器仅仅支持特征点追踪结果，以及IMU量测信息 */
namespace ESKF_VIO_BACKEND {

    /* 某一个特征点于同一时刻在多个相机中的观测 */
    class FeatureObserve {
    public:
        // 特征点在对应 camera ID 中的归一化平面坐标观测
        std::unordered_map<uint32_t, Vector2> norms;
    public:
        FeatureObserve() = default;
        virtual ~FeatureObserve() = default;
        explicit FeatureObserve(const std::unordered_map<uint32_t, Vector2> &norms) :
            norms(norms) {}
    public:
        /* 清空保存的数据 */
        void Clear(void);
        /* 获取指定 camera ID 的观测 */
        bool GetNorm(const uint32_t cameraID, Vector2 &norm);
    };


    /* 特征点追踪结果数据类型定义，保存一帧双目图像的追踪结果 */
    class FeaturesMessage {
    public:
        // 特征点的索引
        std::vector<uint32_t> ids;
        // 归一化平面坐标观测
        std::vector<std::shared_ptr<FeatureObserve>> observes;
        // 标志位
        std::vector<uint8_t> flag;
        // 时间戳（单位 s）
        fp64 timeStamp = 0.0;
    public:
        FeaturesMessage() = default;
        virtual ~FeaturesMessage() = default;
        explicit FeaturesMessage(const std::vector<uint32_t> &ids,
                                 const std::vector<std::shared_ptr<FeatureObserve>> &observes,
                                 const std::vector<uint8_t> &flag,
                                 const fp64 &timeStamp);
    public:
        /* 清空保存的数据 */
        void Clear(void);
        /* 自我打印保存信息 */
        void Information(void);
    };


    /* IMU 量测数据类型定义，保存一次 IMU 量测数据 */
    class IMUMessage {
    public:
        // body 系角速度量测（单位 rad/s）
        Vector3 gyro = Vector3::Zero();
        // body 系加速度量测（单位 m/s^2）
        Vector3 accel = Vector3::Zero();
        // 时间戳（单位 s）
        fp64 timeStamp = 0.0;
    public:
        IMUMessage() = default;
        virtual ~IMUMessage() = default;
        explicit IMUMessage(const Vector3 &gyro,
                            const Vector3 &accel,
                            const fp64 &timeStamp);
    public:
        /* 自我打印保存信息 */
        void Information(void);
    };


    /* 一帧特征点追踪结果和一系列 IMU 量测数据 */
    class CombinedMessage {
    public:
        std::shared_ptr<FeaturesMessage> featMeas;
        std::vector<std::shared_ptr<IMUMessage>> imuMeas;
    public:
        CombinedMessage() = default;
        virtual ~CombinedMessage() = default;
        explicit CombinedMessage(const std::shared_ptr<FeaturesMessage> &featMeas,
                                 const std::shared_ptr<IMUMessage> &imuMeas);
        explicit CombinedMessage(const std::shared_ptr<FeaturesMessage> &featMeas,
                                 const std::vector<std::shared_ptr<IMUMessage>> &imuMeas);
    public:
        /* 清空保存的数据 */
        void Clear(void);
        /* 自我打印保存信息 */
        void Information(void);
    };
}