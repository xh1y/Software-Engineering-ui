#include "appconstants.h"

#include <QCoreApplication>
#include <QString>
#include <QSettings>
#include "logger.h"

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


// ============ 文件处理常量实现 ============

const QString AppConstants::File::JSON_EXTENSION = ".json";
const QString AppConstants::File::CSV_EXTENSION = ".csv";
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

// ============ 应用程序信息实现 ============

const QString AppConstants::AppInfo::CONFIG_ORG = "OptiKG";
const QString AppConstants::AppInfo::CONFIG_APP = "config";

// ============ 可配置参数实现 ============

namespace {
    // 内部可变存储 - 使用Model中的默认值初始化
    int g_chunkSize = 500;    // AppConstants::Model::DEFAULT_CHUNK_SIZE_DEFAULT
    int g_overlapSize = 100;  // AppConstants::Model::DEFAULT_OVERLAP_SIZE_DEFAULT  
    int g_maxChars = 10000;   // AppConstants::Model::MAX_CHARS_DEFAULT
    bool g_initialized = false;
}

void AppConstants::initialize() {
    if (g_initialized) {
        return;
    }
    
    QSettings settings(AppInfo::CONFIG_ORG, AppInfo::CONFIG_APP);
    
    // 从配置文件加载或使用默认值 (Model::DEFAULT_*_DEFAULT)
    g_chunkSize = settings.value("model/chunkSize", 500).toInt();
    g_overlapSize = settings.value("model/overlapSize", 100).toInt();
    g_maxChars = settings.value("model/maxChars", 10000).toInt();
    
    // 验证加载的值
   // LCOV_EXCL_START
    if (!Model::validateChunkSize(g_chunkSize)) {
        Logger::warning(QString("Invalid chunk size loaded from config: %1, using default").arg(g_chunkSize));
        g_chunkSize = 500;
    }
    if (!Model::validateOverlapSize(g_overlapSize)) {
        Logger::warning(QString("Invalid overlap size loaded from config: %1, using default").arg(g_overlapSize));
        g_overlapSize = 100;
    }
    if (!Model::validateMaxChars(g_maxChars)) {
        Logger::warning(QString("Invalid max chars loaded from config: %1, using default").arg(g_maxChars));
        g_maxChars = 10000;
    // LCOV_EXCL_STOP
    }
    
    g_initialized = true;
    Logger::info(QString("AppConstants initialized - chunkSize: %1, overlapSize: %2, maxChars: %3")
                .arg(g_chunkSize).arg(g_overlapSize).arg(g_maxChars));
}

void AppConstants::saveSettings() {
    QSettings settings(AppInfo::CONFIG_ORG, AppInfo::CONFIG_APP);
    settings.setValue("model/chunkSize", g_chunkSize);
    settings.setValue("model/overlapSize", g_overlapSize);
    settings.setValue("model/maxChars", g_maxChars);
    Logger::info("AppConstants settings saved");
}

void AppConstants::resetToDefaults() {
    g_chunkSize = 500;    // DEFAULT_CHUNK_SIZE_DEFAULT
    g_overlapSize = 100;  // DEFAULT_OVERLAP_SIZE_DEFAULT
    g_maxChars = 10000;   // MAX_CHARS_DEFAULT
    saveSettings();
    Logger::info("AppConstants reset to defaults");
}

int AppConstants::chunkSize() { 
    if (!g_initialized) initialize();
    return g_chunkSize; 
}

void AppConstants::setChunkSize(int size) {
    if (Model::validateChunkSize(size)) {
        g_chunkSize = size;
        Logger::info(QString("Chunk size set to: %1").arg(size));
    } else {
        Logger::warning(QString("Attempted to set invalid chunk size: %1").arg(size));
    }
}

int AppConstants::overlapSize() { 
    if (!g_initialized) initialize();
    return g_overlapSize; 
}

void AppConstants::setOverlapSize(int size) {
    if (Model::validateOverlapSize(size)) {
        g_overlapSize = size;
        Logger::info(QString("Overlap size set to: %1").arg(size));
    } else {
        Logger::warning(QString("Attempted to set invalid overlap size: %1").arg(size));
    }
}

int AppConstants::maxChars() { 
    if (!g_initialized) initialize();
    return g_maxChars; 
}

void AppConstants::setMaxChars(int max) {
    if (Model::validateMaxChars(max)) {
        g_maxChars = max;
        Logger::info(QString("Max chars set to: %1").arg(max));
    } else {
        Logger::warning(QString("Attempted to set invalid max chars: %1").arg(max));
    }
}

int& AppConstants::Model::DEFAULT_CHUNK_SIZE() {
    if (!g_initialized) initialize();
    return g_chunkSize;
}

int& AppConstants::Model::DEFAULT_OVERLAP_SIZE() {
    if (!g_initialized) initialize();
    return g_overlapSize;
}

int& AppConstants::Model::MAX_CHARS() {
    if (!g_initialized) initialize();
    return g_maxChars;
}

bool AppConstants::Model::validateChunkSize(int size) {
    return size >= 100 && size <= 5000;
}

bool AppConstants::Model::validateOverlapSize(int size) {
    return size >= 0 && size <= 1000;
}

bool AppConstants::Model::validateMaxChars(int max) {
    return max >= 1000 && max <= 100000;
}

} // namespace optikg