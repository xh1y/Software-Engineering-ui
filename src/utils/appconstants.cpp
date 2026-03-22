#include "appconstants.h"

#include <QDir>
#include <QCoreApplication>
#include <QString>
#include <QColor>
#include <QStandardPaths>
#include <QProcessEnvironment>

namespace optikg {

// ============ UI 常量实现 ============

const QString AppConstants::UI::ERROR_COLOR = "#f44336";
const QString AppConstants::UI::SUCCESS_COLOR = "#4CAF50";
const QString AppConstants::UI::WARNING_COLOR = "#FF9800";
const QString AppConstants::UI::INFO_COLOR = "#666";

const QString AppConstants::UI::EXTRACT_BTN_COLOR = "#4CAF50";
const QString AppConstants::UI::EXTRACT_BTN_HOVER = "#388E3C";
const QString AppConstants::UI::EXTRACT_BTN_DISABLED = "#BDBDBD";
const QString AppConstants::UI::CLEAR_BTN_COLOR = "#f44336";
const QString AppConstants::UI::CLEAR_BTN_HOVER = "#d32f2f";
const QString AppConstants::UI::BATCH_DELETE_BTN_COLOR = "#f44336";

const QString AppConstants::UI::CHAR_COUNT_COLOR = "#666";
const QString AppConstants::UI::CHAR_COUNT_WARNING = "#FF9800";
const QString AppConstants::UI::CHAR_COUNT_ERROR = "#f44336";

// ============ 模型处理常量实现 ============

const QString AppConstants::Model::MODEL_FILE_EXTENSION = ".onnx";
const QString AppConstants::Model::TOKENIZER_FILE_NAME = "tokenizer.json";
const QString AppConstants::Model::METADATA_FILE_NAME = "metadata.json";
const QString AppConstants::Model::VOCAB_FILE_NAME = "vocab.txt";

QStringList AppConstants::Model::getModelSearchPaths(const QString& appDir) {
    QStringList candidates;

    // 优先尝试原始模型 (model_fp32.onnx)
    // 先尝试 model/ 目录，再尝试 models/ 目录
    QDir dir(appDir);
    candidates << dir.filePath("model/model_fp32.onnx");
    candidates << dir.filePath("../model/model_fp32.onnx");
    candidates << dir.filePath("../../model/model_fp32.onnx");
    candidates << "model/model_fp32.onnx";
    candidates << dir.filePath("models/model_fp32.onnx");
    candidates << dir.filePath("../models/model_fp32.onnx");
    candidates << dir.filePath("../../models/model_fp32.onnx");
    candidates << "models/model_fp32.onnx";

    // 用户数据目录（跨平台）
    QString userDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    candidates << QDir(userDataDir).filePath("model/model_fp32.onnx");
    candidates << QDir(userDataDir).filePath("models/model_fp32.onnx");

#ifdef _WIN32
    // Windows 特定路径
    QString programData = qEnvironmentVariable("ProgramData");
    if (!programData.isEmpty()) {
        candidates << QDir(programData).filePath("OptiKG/model/model_fp32.onnx");
        candidates << QDir(programData).filePath("OptiKG/models/model_fp32.onnx");
    }

    QString appData = qEnvironmentVariable("APPDATA");
    if (!appData.isEmpty()) {
        candidates << QDir(appData).filePath("OptiKG/model/model_fp32.onnx");
        candidates << QDir(appData).filePath("OptiKG/models/model_fp32.onnx");
    }

    // 程序安装目录（假设安装在Program Files）
    QString programFiles = qEnvironmentVariable("ProgramFiles");
    if (!programFiles.isEmpty()) {
        candidates << QDir(programFiles).filePath("OptiKG/model/model_fp32.onnx");
        candidates << QDir(programFiles).filePath("OptiKG/models/model_fp32.onnx");
    }
#else
    // Linux/macOS 系统路径
    candidates << "/usr/share/optikg/model/model_fp32.onnx";
    candidates << "/usr/local/share/optikg/model/model_fp32.onnx";
    candidates << "/opt/optikg/model/model_fp32.onnx";
    candidates << "/opt/optikg/models/model_fp32.onnx";

    // macOS 应用包路径
#ifdef __APPLE__
    candidates << QDir(QCoreApplication::applicationDirPath()).filePath("../Resources/model/model_fp32.onnx");
    candidates << QDir(QCoreApplication::applicationDirPath()).filePath("../Resources/models/model_fp32.onnx");
#endif
#endif

    return candidates;
}

// ============ 文件处理常量实现 ============

const QString AppConstants::File::JSON_EXTENSION = ".json";
const QString AppConstants::File::CSV_EXTENSION = ".csv";
const QString AppConstants::File::ALL_FILES_FILTER = "*.*";

const QString AppConstants::File::JSON_FILE_FILTER = QCoreApplication::translate("AppConstants", "JSON文件 (*.json)");
const QString AppConstants::File::CSV_FILE_FILTER = QCoreApplication::translate("AppConstants", "CSV文件 (*.csv)");
const QString AppConstants::File::ONNX_FILE_FILTER = QCoreApplication::translate("AppConstants", "ONNX 模型 (*.onnx)");

QStringList AppConstants::File::getCsvEncodings() {
    return {
        "UTF-8",
        "GBK",
        "GB2312",
        "GB18030",
        "ISO-8859-1"
    };
}

QStringList AppConstants::File::getExportFormats() {
    return {
        "JSON",
        "CSV"
    };
}

// ============ 实体类型常量实现 ============

QMap<QString, AppConstants::EntityTypeInfo> AppConstants::getEntityTypeMap() {
    static QMap<QString, EntityTypeInfo> typeMap;

    if (typeMap.isEmpty()) {
        typeMap["部件"] = {"部件", QColor(255, 107, 107), "#FF6B6B"};
        typeMap["故障"] = {"故障", QColor(78, 205, 196), "#4ECDC4"};
        typeMap["工具"] = {"工具", QColor(255, 193, 7), "#FFC107"};
        typeMap["组成"] = {"组成", QColor(156, 39, 176), "#9C27B0"};
    }

    return typeMap;
}

// ============ 应用程序信息实现 ============

const QString AppConstants::AppInfo::APP_NAME = "OptiKG";
const QString AppConstants::AppInfo::APP_DISPLAY_NAME = QCoreApplication::translate("AppConstants", "OptiKG - 工业知识抽取系统");
const QString AppConstants::AppInfo::APP_VERSION = "1.0.0";
const QString AppConstants::AppInfo::ORG_NAME = "OptiKG";
const QString AppConstants::AppInfo::ORG_DOMAIN = "optikg.org";

const QString AppConstants::AppInfo::CONFIG_ORG = "OptiKG";
const QString AppConstants::AppInfo::CONFIG_APP = "config";

} // namespace optikg