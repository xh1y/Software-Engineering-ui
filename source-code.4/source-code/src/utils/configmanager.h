#pragma once

#include <QObject>
#include <QString>
#include <QSettings>
#include <QMap>
#include "appconstants.h"

namespace optikg {

/**
 * @brief 配置管理类（单例模式）
 * 
 * 负责管理应用的所有配置设置，包括：
 * - 字段映射设置（JSON/CSV 内容字段名）
 * - 模型路径和参数
 * - 界面设置
 * 
 * 配置存储在 QSettings 中，持久化到系统配置目录
 */
class ConfigManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取配置管理器单例实例
     * @return ConfigManager& 单例引用
     */
    static ConfigManager& instance() {
        static ConfigManager instance;
        return instance;
    }

    // 禁用拷贝和赋值
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    /**
     * @brief 初始化配置管理器，加载所有设置
     * @return true 初始化成功
     */
    bool initialize();

    // ============ 字段映射设置 ============
    
    /**
     * @brief 设置 JSON 内容字段名
     * @param fieldName 字段名称，默认为 "text"
     */
    void setJsonContentField(const QString& fieldName);
    
    /**
     * @brief 获取 JSON 内容字段名
     * @return QString 字段名称
     */
    QString getJsonContentField() const;

    /**
     * @brief 设置 CSV 内容列名
     * @param columnName 列名称，默认为 "content"
     */
    void setCsvContentColumn(const QString& columnName);
    
    /**
     * @brief 获取 CSV 内容列名
     * @return QString 列名称
     */
    QString getCsvContentColumn() const;

    /**
     * @brief 设置 CSV 编码格式
     * @param encoding 编码名称，默认为 "UTF-8"
     */
    void setCsvEncoding(const QString& encoding);
    
    /**
     * @brief 获取 CSV 编码格式
     * @return QString 编码名称
     */
    QString getCsvEncoding() const;

    // ============ 模型设置 ============
    
    /**
     * @brief 设置模型路径
     * @param path 模型文件路径
     */
    void setModelPath(const QString& path);
    
    /**
     * @brief 获取模型路径
     * @return QString 模型路径
     */
    QString getModelPath() const;
    
    /**
     * @brief 自动检测并获取模型路径
     *
     * 搜索顺序：
     * 1. 已配置的模型路径
     * 2. 可执行文件目录下的 model/model_fp32.onnx 或 models/model_fp32.onnx
     * 3. 项目目录下的 model/model_fp32.onnx 或 models/model_fp32.onnx
     *
     * @return QString 检测到的模型路径，未找到返回空
     */
    QString autoDetectModelPath();

    /**
     * @brief 设置置信度阈值
     * @param threshold 阈值，默认为 -10.0
     */
    void setThreshold(float threshold);
    
    /**
     * @brief 获取置信度阈值
     * @return float 阈值
     */
    float getThreshold() const;

    // ============ 批量处理设置 ============
    
    /**
     * @brief 设置批量处理输出目录
     * @param path 输出目录路径
     */
    void setBatchOutputDir(const QString& path);
    
    /**
     * @brief 获取批量处理输出目录
     * @return QString 输出目录路径
     */
    QString getBatchOutputDir() const;

    /**
     * @brief 设置是否自动导出结果
     * @param autoExport 是否自动导出
     */
    void setAutoExport(bool autoExport);
    
    /**
     * @brief 获取是否自动导出结果
     * @return true 自动导出
     */
    bool getAutoExport() const;

    /**
     * @brief 设置默认导出格式
     * @param format 格式字符串 "json" 或 "csv"
     */
    void setDefaultExportFormat(const QString& format);
    
    /**
     * @brief 获取默认导出格式
     * @return QString 格式字符串
     */
    QString getDefaultExportFormat() const;

    /**
     * @brief 设置是否保存到数据库
     * @param save 是否保存
     */
    void setSaveToDatabase(bool save);

    /**
     * @brief 获取是否保存到数据库
     * @return true 保存到数据库
     */
    bool getSaveToDatabase() const;

    // ============ 界面设置 ============
    
    /**
     * @brief 设置窗口几何信息
     * @param geometry 窗口几何数据
     */
    void setWindowGeometry(const QByteArray& geometry);
    
    /**
     * @brief 获取窗口几何信息
     * @return QByteArray 窗口几何数据
     */
    QByteArray getWindowGeometry() const;

    /**
     * @brief 保存所有设置到持久化存储
     */
    void saveSettings();

signals:
    /**
     * @brief 设置变更信号
     * @param key 变更的设置键
     */
    void settingChanged(const QString& key);

private:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager() override;

    QSettings* settings_ = nullptr;
    bool initialized_ = false;

    // 缓存的配置值
    QString jsonContentField_;
    QString csvContentColumn_;
    QString csvEncoding_;
    QString modelPath_;
    QString batchOutputDir_;
    QString defaultExportFormat_;
    float threshold_ = AppConstants::Model::DEFAULT_THRESHOLD;
    bool autoExport_ = false;
    bool saveToDatabase_ = false;
};

} // namespace optikg
