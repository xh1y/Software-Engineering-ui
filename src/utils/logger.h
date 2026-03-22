#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

namespace optikg {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    Debug,    ///< 调试信息
    Info,     ///< 一般信息
    Warning,  ///< 警告信息
    Error,    ///< 错误信息
    Critical  ///< 严重错误
};

/**
 * @brief 日志管理类（单例模式）
 *
 * 负责将日志写入文件，支持不同日志级别，包含时间戳。
 * 日志文件位于用户配置目录下的 logs 子目录中。
 * 默认日志级别为 Info，可通过 setLogLevel 调整。
 */
class Logger {

public:
    /**
     * @brief 获取日志管理器单例实例
     * @return Logger& 单例引用
     */
    static Logger& instance();

    // 禁用拷贝和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief 初始化日志系统
     * @param logDir 日志目录，如果为空则使用默认目录
     * @param fileName 日志文件名（不含路径），如果为空则使用 "optikg_yyyy-MM-dd.log"
     * @return true 初始化成功
     */
    bool initialize(const QString& logDir = QString(), const QString& fileName = QString());

    /**
     * @brief 设置日志级别
     * @param level 最低输出日志级别
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief 获取当前日志级别
     * @return LogLevel 当前日志级别
     */
    LogLevel logLevel() const;

    /**
     * @brief 写日志
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(LogLevel level, const QString& message);

    /**
     * @brief 写调试日志
     * @param message 日志消息
     */
    static void debug(const QString& message);

    /**
     * @brief 写信息日志
     * @param message 日志消息
     */
    static void info(const QString& message);

    /**
     * @brief 写警告日志
     * @param message 日志消息
     */
    static void warning(const QString& message);

    /**
     * @brief 写错误日志
     * @param message 日志消息
     */
    static void error(const QString& message);

    /**
     * @brief 写严重错误日志
     * @param message 日志消息
     */
    static void critical(const QString& message);

    /**
     * @brief 关闭日志系统
     */
    void shutdown();

    /**
     * @brief 安装 Qt 消息处理器，将所有 qDebug/qWarning 等输出重定向到日志文件
     * @param enableConsoleOutput 是否同时输出到控制台（默认 false，即不输出到控制台）
     */
    static void installQtMessageHandler(bool enableConsoleOutput = false);

    /**
     * @brief 恢复默认的 Qt 消息处理器
     */
    static void restoreDefaultMessageHandler();


private:
    explicit Logger();
    ~Logger();

    QString levelToString(LogLevel level) const;
    QString getDefaultLogDir() const;
    QString generateDefaultFileName() const;
    void writeToFile(const QString& logLine);

    static void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    QFile logFile_;
    QTextStream logStream_;
    mutable QMutex mutex_;
    LogLevel currentLevel_ = LogLevel::Info;
    bool initialized_ = false;

    static bool s_enableConsoleOutput;
};

} // namespace optikg