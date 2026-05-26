#pragma once

#include <QString>
#include <QMap>

namespace optikg {

/**
 * @brief 样式管理器
 * 
 * 管理应用程序的 QSS 样式，支持加载、切换主题
 */
class StyleManager {
public:
    /**
     * @brief 获取单例实例
     */
    static StyleManager& instance();

    /**
     * @brief 初始化并加载默认样式
     */
    void initialize();

    /**
     * @brief 从文件加载样式表
     * @param filePath QSS 文件路径
     * @return 是否成功加载
     */
    bool loadStyleSheet(const QString& filePath);

    /**
     * @brief 从资源文件加载样式表
     * @param resourcePath 资源路径，如 ":/styles/modern_light.qss"
     * @return 是否成功加载
     */
    bool loadStyleSheetFromResource(const QString& resourcePath);

    /**
     * @brief 应用样式字符串
     * @param styleSheet 样式表内容
     */
    void applyStyleSheet(const QString& styleSheet);

    /**
     * @brief 获取当前样式表
     */
    QString currentStyleSheet() const { return currentStyleSheet_; }

    /**
     * @brief 获取最后的错误信息
     */
    QString lastError() const { return lastError_; }

    /**
     * @brief 重新加载当前主题（用于强制刷新）
     */
    bool reloadCurrentTheme();

private:
    StyleManager() = default;
    ~StyleManager() = default;
    StyleManager(const StyleManager&) = delete;
    StyleManager& operator=(const StyleManager&) = delete;

    QString currentStyleSheet_;
    QString lastError_;
};

} // namespace optikg
