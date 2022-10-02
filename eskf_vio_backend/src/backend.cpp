/* 内部依赖 */
#include <backend.hpp>
/* 外部依赖 */

namespace ESKF_VIO_BACKEND {
    /* 输入一帧 IMU 数据 */
    bool Backend::GetIMUMessage(const std::shared_ptr<IMUMessage> &newImuMeas) {
        this->dataloader.PushIMUMessage(newImuMeas);
        return true;
    }


    /* 输入一帧 Features 追踪数据 */
    bool Backend::GetFeaturesMessage(const std::shared_ptr<FeaturesMessage> &newFeatMeas) {
        this->dataloader.PushFeaturesMessage(newFeatMeas);
        return true;
    }


    /* 单步运行 */
    bool Backend::RunOnce(void) {
        bool res = true;
        // 提取一个数据，可能是单个 IMU 数据，也可能是 IMU 和 features 的捆绑数据
        std::shared_ptr<CombinedMessage> msg;
        res = this->dataloader.PopOneMessage(msg);
        if (res == false) {
            return false;
        }

        // 当存在 IMU 量测输入时
        if (msg->imuMeas.size() > 0) {
            if (msg->imuMeas.front() != nullptr) {
                // 姿态解算求解器始终保持工作
                res = this->attitudeEstimator.Propagate(msg->imuMeas.front()->accel,
                                                        msg->imuMeas.front()->gyro,
                                                        msg->imuMeas.front()->timeStamp);
                // 状态递推器仅在完成初始化之后工作
                if (this->status != NEED_INIT) {
                    res = this->propagator.Propagate(msg->imuMeas.front()->accel,
                                                     msg->imuMeas.front()->gyro,
                                                     msg->imuMeas.front()->timeStamp);
                }
            }
        }

        // 当存在特征点追踪结果输入时
        if (msg->featMeas != nullptr) {
            // 更新特征点管理器和帧管理器
            res = this->UpdateFeatureFrameManager(msg->featMeas);

            if (this->status == NEED_INIT) {
                // 在没有进行初始化时，仅在存在连续两帧观测时才进行初始化
                if (this->frameManager.frames.size() >= 2) {
                    this->status = INITIALIZING;
                    // Step 1: 从 attitude estimator 中拿取此帧对应时刻的姿态估计结果，赋值给首帧的 q_wb

                    // Step 2: 首帧观测为 multi-view 时，进行首帧内的多目三角测量，得到一些特征点的 p_w

                    // Step 3: 利用这些特征点在下一帧中的观测，通过重投影 PnP 迭代，估计出下一帧基于首帧参考系 w 系的相对位置

                    // Step 4: 两帧位置对时间进行差分，得到首帧在 w 系中的速度估计，至此首帧对应时刻点的 q_wb 和 v_wb 都已经确定

                    // Step 5: 首帧位置设置为原点，将首帧位姿和速度赋值给 propagator 的 initState

                    // Step 6: 从 attitude estimator 提取出从首帧时刻点开始，到最新时刻的 imu 量测，输入到 propagator 让他递推到最新时刻

                    this->status = INITIALIZED;
                }
                // Step e1: 如果初始化失败，删掉最旧帧，使得滑动窗口内仅剩下一帧

                // Step e2: 从下一帧的时刻开始，清空 attitude estimator 中记录的 imu 历史量测数据和姿态估计结果

            } else {
                // 在已经完成初始化的情况下，进行一次 update 的流程
                // Step 1: 定位到 propagator 序列中对应时间戳的地方，提取对应时刻状态，清空在这之前的序列 item

                // Step 2: 若此时滑动窗口已满，则判断最新帧是否为关键帧，确定边缘化策略。可以在此时给前端 thread 发送信号。

                // Step 3: 扩展 state 的维度以及协方差矩阵，利用对应时刻的 propagate 名义状态给 frame pose 赋值

                // Step 4: 三角测量滑动窗口内所有特征点。已被测量过的选择迭代法，没被测量过的选择数值法。更新每一个点的三角测量质量。

                // Step 5: 基于三角测量的质量，选择一定数量的特征点来构造量测方程。其中包括计算雅可比、投影到左零空间、缩减维度、卡尔曼 update 误差和名义状态

                // Step 6: 根据边缘化策略，选择跳过此步，或裁减 update 时刻点上的状态和协方差矩阵

                // Step 7: 对于 propagate queue 中后续的已经存在的 items，从 update 时刻点重新 propagate

                // Step 8: 基于边缘化策略，调整特征点管理器和关键帧管理器的管理内容
                if (this->frameManager.NeedMarginalize() == true) {
                    this->MarginalizeFeatureFrameManager(MARG_OLDEST);
                }
            }
        }

        return res;
    }


    /* 输出最新 Propagate 点估计 */
    bool Backend::PublishPropagateState(IMUFullState &state) {
        if (this->propagator.items.empty()) {
            return false;
        }
        state.p_wb = this->propagator.items.back()->nominalState.p_wb;
        state.q_wb = this->propagator.items.back()->nominalState.q_wb;
        state.v_wb = this->propagator.items.back()->nominalState.v_wb;
        state.v_wb = this->propagator.items.back()->nominalState.v_wb;
        state.v_wb = this->propagator.items.back()->nominalState.v_wb;
        state.bias_a = this->propagator.bias_a;
        state.bias_g = this->propagator.bias_g;
        return true;
    }


    /* 输出最新 Update 点估计 */
    bool Backend::PublishUpdateState(IMUFullState &state) {
        if (this->propagator.items.empty()) {
            return false;
        }
        state.p_wb = this->propagator.items.front()->nominalState.p_wb;
        state.q_wb = this->propagator.items.front()->nominalState.q_wb;
        state.v_wb = this->propagator.items.front()->nominalState.v_wb;
        state.bias_a = this->propagator.bias_a;
        state.bias_g = this->propagator.bias_g;
        return true;
    }


    /* 重置 */
    void Backend::Reset(void) {
        this->status = NEED_INIT;
    }


    /* 将新输入的 feature message 更新到特征点管理器和帧管理器中 */
    bool Backend::UpdateFeatureFrameManager(const std::shared_ptr<FeaturesMessage> &featMeas) {
        if (featMeas == nullptr) {
            return false;
        }
        // 添加新帧，新帧 frame id 由 frame manager 给定
        std::shared_ptr<Frame> newFrame(new Frame(featMeas->timeStamp));
        this->frameManager.AddNewFrame(newFrame);

        // 特征点追踪结果添加到 feature manager
        std::vector<std::shared_ptr<Feature>> newObserveFeatures;
        this->featureManager.AddNewFeatures(featMeas->ids, featMeas->observes,
            newFrame->id, newObserveFeatures);

        // 新帧关联特征点追踪结果
        newFrame->AddFeatures(newObserveFeatures);
        return true;
    }


    /* 基于 marg 策略调整特征点管理器和帧管理器 */
    bool Backend::MarginalizeFeatureFrameManager(MargPolicy policy) {
        switch (policy) {
            case MARG_NEWEST:
                // 剔除最新帧以及相关特征点
                this->featureManager.RemoveByFrameID(this->frameManager.frames.back()->id, true);
                this->frameManager.RemoveFrame(this->frameManager.frames.size() - 1);
                break;
            case MARG_OLDEST:
                // 剔除最旧帧以及相关特征点
                this->featureManager.RemoveByFrameID(this->frameManager.frames.front()->id, false);
                this->frameManager.RemoveFrame(0);
                break;
            case MARG_SUBNEW:
                // 剔除次新帧以及相关特征点
                this->featureManager.RemoveByFrameID(this->frameManager.frames.back()->id - 1, true);
                this->frameManager.RemoveFrame(this->frameManager.frames.size() - 2);
                break;
            default:
                return false;
        }
        return true;
    }


    /* 设置相机与 IMU 之间的外参 */
    bool Backend::SetExtrinsic(const std::vector<Quaternion> &q_bc,
        const std::vector<Vector3> &p_bc) {
        if (q_bc.size() != p_bc.size()) {
            return false;
        } else {
            this->q_bc = q_bc;
            this->p_bc = p_bc;
            return true;
        }
    }
}