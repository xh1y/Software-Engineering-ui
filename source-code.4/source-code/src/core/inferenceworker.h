#pragma once

#include <QThread>
#include <QString>
#include <memory>
#include "optikg/datamodel.h"

// ONNX Runtime headers
#include <onnxruntime_cxx_api.h>
#include <tokenizers_cpp.h>

namespace optikg {

class InferenceWorker : public QThread {
    Q_OBJECT

public:
    explicit InferenceWorker(QObject *parent = nullptr);
    ~InferenceWorker() override;

    void setText(const QString& text);
    void setModelPath(const QString& path);
    void setThreshold(float threshold);

    void stop(); // 请求停止

signals:
    void progressChanged(int value);
    void resultReady(const QList<Triple>& results);
    void errorOccurred(const QString& error);

protected:
    void run() override;

private:
    QString text_;
    QString modelPath_;
    float threshold_ = -10.0f;
    std::atomic<bool> stopRequested_{false};

    // ONNX Runtime components
    Ort::Env env_{nullptr};
    std::unique_ptr<Ort::Session> session_;
    Ort::AllocatorWithDefaultOptions allocator_;
    bool modelLoaded_ = false;

    // Tokenizer 和模型信息
    std::unique_ptr<tokenizers::Tokenizer> tokenizer_;
    int maxSeqLength_ = 512;
    QMap<int, QString> id2predicate_;

    // 辅助方法
    bool loadModel();
    bool loadTokenizer(const QString& path);
    bool loadMetadata(const QString& metadataPath);
    QList<Triple> runOnnxInference(const QString& text);
    QList<Triple> tokenizeAndPredict(const QString& text);
    QList<Triple> simulateInference(const QString& text);
    QList<Triple> inferWithChunking(const QString& text, int chunkSize = 500, int overlapSize = 100);
    void cleanup();
};

} // namespace optikg
