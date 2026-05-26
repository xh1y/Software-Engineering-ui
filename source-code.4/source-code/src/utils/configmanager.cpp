#include "configmanager.h"
#include "appconstants.h"

#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include "logger.h"
#include <QCoreApplication>
#include <QFileInfo>

namespace optikg {

// 配置键名常量
namespace Keys {
    constexpr const char* JSON_CONTENT_FIELD = "fieldMapping/jsonContentField";
    constexpr const char* CSV_CONTENT_COLUMN = "fieldMapping/csvContentColumn";
    constexpr const char* CSV_ENCODING = "fieldMapping/csvEncoding";
    constexpr const char* MODEL_PATH = "model/path";
    constexpr const char* THRESHOLD = "model/threshold";
    constexpr const char* BATCH_OUTPUT_DIR = "batch/outputDir";
    constexpr const char* AUTO_EXPORT = "batch/autoExport";
    constexpr const char* DEFAULT_EXPORT_FORMAT = "batch/defaultExportFormat";
    constexpr const char* SAVE_TO_DATABASE = "batch/saveToDatabase";
    constexpr const char* WINDOW_GEOMETRY = "ui/windowGeometry";
}


ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , settings_(nullptr)
    , initialized_(false) {
}

ConfigManager::~ConfigManager() {
    if (settings_) {
        settings_->sync();
        delete settings_;
    }
}

bool ConfigManager::initialize() {
    if (initialized_) {
        return true;
    }

    // 使用系统标准配置目录
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir;
    if (!dir.exists(configDir)) {
        if (!dir.mkpath(configDir)) {
            Logger::warning(QString("Failed to create config directory: %1").arg(configDir));
            // 继续使用默认位置
        }
    }

    // 初始化 QSettings
    settings_ = new QSettings(
        QSettings::IniFormat,
        QSettings::UserScope,
        "OptiKG",
        "config",
        this
    );

    // 加载所有配置到缓存
    jsonContentField_ = settings_->value(Keys::JSON_CONTENT_FIELD, AppConstants::Mapping::DEFAULT_JSON_FIELD).toString();
    csvContentColumn_ = settings_->value(Keys::CSV_CONTENT_COLUMN, AppConstants::Mapping::DEFAULT_CSV_COLUMN).toString();
    csvEncoding_ = settings_->value(Keys::CSV_ENCODING, AppConstants::Mapping::DEFAULT_CSV_ENCODING).toString();
    modelPath_ = settings_->value(Keys::MODEL_PATH, QString()).toString();
    threshold_ = settings_->value(Keys::THRESHOLD, AppConstants::Model::DEFAULT_THRESHOLD).toFloat();
    batchOutputDir_ = settings_->value(Keys::BATCH_OUTPUT_DIR, QString()).toString();
    autoExport_ = settings_->value(Keys::AUTO_EXPORT, false).toBool();
    defaultExportFormat_ = settings_->value(Keys::DEFAULT_EXPORT_FORMAT, AppConstants::Mapping::DEFAULT_EXPORT_FORMAT).toString();
    saveToDatabase_ = settings_->value(Keys::SAVE_TO_DATABASE, false).toBool();

    initialized_ = true;
    Logger::debug("ConfigManager initialized successfully");
    return true;
}

// ============ 字段映射设置 ============

void ConfigManager::setJsonContentField(const QString& fieldName) {
    if (jsonContentField_ != fieldName) {
        jsonContentField_ = fieldName;
        settings_->setValue(Keys::JSON_CONTENT_FIELD, fieldName);
        emit settingChanged(Keys::JSON_CONTENT_FIELD);
    }
}

QString ConfigManager::getJsonContentField() const {
    return jsonContentField_;
}

void ConfigManager::setCsvContentColumn(const QString& columnName) {
    if (csvContentColumn_ != columnName) {
        csvContentColumn_ = columnName;
        settings_->setValue(Keys::CSV_CONTENT_COLUMN, columnName);
        emit settingChanged(Keys::CSV_CONTENT_COLUMN);
    }
}

QString ConfigManager::getCsvContentColumn() const {
    return csvContentColumn_;
}

void ConfigManager::setCsvEncoding(const QString& encoding) {
    if (csvEncoding_ != encoding) {
        csvEncoding_ = encoding;
        settings_->setValue(Keys::CSV_ENCODING, encoding);
        emit settingChanged(Keys::CSV_ENCODING);
    }
}

QString ConfigManager::getCsvEncoding() const {
    return csvEncoding_;
}

// ============ 模型设置 ============

void ConfigManager::setModelPath(const QString& path) {
    if (modelPath_ != path) {
        modelPath_ = path;
        settings_->setValue(Keys::MODEL_PATH, path);
        emit settingChanged(Keys::MODEL_PATH);
    }
}

QString ConfigManager::getModelPath() const {
    return modelPath_;
}

QString ConfigManager::autoDetectModelPath() {
    // 1. 如果已配置且文件存在，直接返回
    if (!modelPath_.isEmpty() && QFile::exists(modelPath_)) {
        return modelPath_;
    }

    // 2. 尝试查找 model_fp32.onnx (原始模型)
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates;

    // 优先尝试原始模型 (model_fp32.onnx)
    // 先尝试 model/ 目录，再尝试 models/ 目录
    candidates << QDir(appDir).filePath("model/model_fp32.onnx");
    candidates << QDir(appDir).filePath("../model/model_fp32.onnx");
    candidates << QDir(appDir).filePath("../../model/model_fp32.onnx");
    candidates << "model/model_fp32.onnx";
    candidates << QDir(appDir).filePath("models/model_fp32.onnx");
    candidates << QDir(appDir).filePath("../models/model_fp32.onnx");
    candidates << QDir(appDir).filePath("../../models/model_fp32.onnx");
    candidates << "models/model_fp32.onnx";


    for (const QString& path : candidates) {
        QFileInfo info(path);
        if (info.exists() && info.isFile()) {
            QString detectedPath = info.absoluteFilePath();
            Logger::debug(QString("✓ Using original model (model_fp32.onnx) at: %1").arg(detectedPath));

            // 自动保存检测到的路径
            setModelPath(detectedPath);
            return detectedPath;
        }
    }

    Logger::warning("Could not auto-detect model file");
    return QString();
}

void ConfigManager::setThreshold(float threshold) {
    if (!qFuzzyCompare(threshold_, threshold)) {
        threshold_ = threshold;
        settings_->setValue(Keys::THRESHOLD, threshold);
        emit settingChanged(Keys::THRESHOLD);
    }
}

float ConfigManager::getThreshold() const {
    return threshold_;
}

// ============ 批量处理设置 ============

void ConfigManager::setBatchOutputDir(const QString& path) {
    if (batchOutputDir_ != path) {
        batchOutputDir_ = path;
        settings_->setValue(Keys::BATCH_OUTPUT_DIR, path);
        emit settingChanged(Keys::BATCH_OUTPUT_DIR);
    }
}

QString ConfigManager::getBatchOutputDir() const {
    return batchOutputDir_;
}

void ConfigManager::setAutoExport(bool autoExport) {
    if (autoExport_ != autoExport) {
        autoExport_ = autoExport;
        settings_->setValue(Keys::AUTO_EXPORT, autoExport);
        emit settingChanged(Keys::AUTO_EXPORT);
    }
}

bool ConfigManager::getAutoExport() const {
    return autoExport_;
}

void ConfigManager::setDefaultExportFormat(const QString& format) {
    if (defaultExportFormat_ != format) {
        defaultExportFormat_ = format;
        settings_->setValue(Keys::DEFAULT_EXPORT_FORMAT, format);
        emit settingChanged(Keys::DEFAULT_EXPORT_FORMAT);
    }
}

QString ConfigManager::getDefaultExportFormat() const {
    return defaultExportFormat_;
}

void ConfigManager::setSaveToDatabase(bool save) {
    if (saveToDatabase_ != save) {
        saveToDatabase_ = save;
        settings_->setValue(Keys::SAVE_TO_DATABASE, save);
        emit settingChanged(Keys::SAVE_TO_DATABASE);
    }
}

bool ConfigManager::getSaveToDatabase() const {
    return saveToDatabase_;
}

// ============ 界面设置 ============

void ConfigManager::setWindowGeometry(const QByteArray& geometry) {
    settings_->setValue(Keys::WINDOW_GEOMETRY, geometry);
}

QByteArray ConfigManager::getWindowGeometry() const {
    return settings_->value(Keys::WINDOW_GEOMETRY).toByteArray();
}

void ConfigManager::saveSettings() {
    if (settings_) {
        settings_->sync();
        Logger::debug("Settings saved");
    }
}

} // namespace optikg
