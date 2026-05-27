#pragma once

#include <QString>
#include <QMap>
#include <memory>
#include "optikg/datamodel.h"

// ONNX Runtime headers
#include <onnxruntime_cxx_api.h>
#include <tokenizers_cpp.h>

namespace optikg {

/**
 * @brief 元数据加载结果枚举
 */
enum class MetadataLoadResult {
    Success,                ///< 加载成功
    FileNotFound,          ///< 文件不存在
    InvalidJson            ///< JSON格式无效
};

/**
 * @brief 推理引擎 - 同步执行知识抽取
 * 
 * 独立于 QThread 的推理引擎，可被 BatchProcessor 和 InferenceWorker 复用
 */
class InferenceEngine {
public:
    InferenceEngine();
    ~InferenceEngine();

    /**
     * @brief 加载模型
     * @param modelPath ONNX 模型路径
     * @return true 加载成功
     */
    bool loadModel(const QString& modelPath);

    /**
     * @brief 检查模型是否已加载
     * @return true 已加载
     */
    bool isModelLoaded() const { return modelLoaded_; }

    /**
     * @brief 设置置信度阈值
     * @param threshold 阈值
     */
    void setThreshold(float threshold) { threshold_ = threshold; }

    /**
     * @brief 执行推理
     * @param text 输入文本
     * @return QList<Triple> 抽取的三元组
     */
    QList<Triple> infer(const QString& text);

    /**
     * @brief 执行长文本推理（分块处理）
     * @param text 输入文本
     * @param chunkSize 每块最大字符数（默认2000）
     * @param overlapSize 块间重叠字符数（默认400）
     * @return QList<Triple> 抽取的三元组
     */
    QList<Triple> inferLongText(const QString& text, int chunkSize = 500, int overlapSize = 100);

    /**
     * @brief 卸载模型，释放资源
     */
    void unload();

private:
    bool loadTokenizer(const QString& path);
    MetadataLoadResult loadMetadata(const QString& metadataPath);
    QList<Triple> runInference(const QString& text);
    void cleanup();

    // ONNX Runtime components
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    Ort::AllocatorWithDefaultOptions allocator_;
    bool modelLoaded_ = false;

    // Tokenizer 和模型信息
    std::unique_ptr<tokenizers::Tokenizer> tokenizer_;
    int maxSeqLength_ = 128;
    float threshold_ = -10.0f;
    QMap<int, QString> id2predicate_;
    QString modelPath_;
};

} // namespace optikg
