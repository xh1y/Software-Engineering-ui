#include "logger.h"

#include <QDebug>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QIODevice>

namespace optikg {
    bool Logger::s_enableConsoleOutput = false;

    Logger &Logger::instance() {
        static Logger instance;
        return instance;
    }

    Logger::Logger()
        : currentLevel_(LogLevel::Info)
          , initialized_(false) {
    }

    Logger::~Logger() {
        shutdown();
    }

    bool Logger::initialize(const QString &logDir, const QString &fileName) {
        QMutexLocker locker(&mutex_);

        if (initialized_) {
            return true;
        }

        // 确定日志目录
        QString targetLogDir = logDir;
        if (targetLogDir.isEmpty()) {
            targetLogDir = getDefaultLogDir();
        }

        // 创建日志目录
        QDir dir(targetLogDir);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                qWarning() << "Failed to create log directory:" << targetLogDir;
                return false;
            }
        }

        // 确定日志文件名
        QString targetFileName = fileName;
        if (targetFileName.isEmpty()) {
            targetFileName = generateDefaultFileName();
        }

        QString filePath = dir.absoluteFilePath(targetFileName);

        // 打开日志文件（追加模式）
        logFile_.setFileName(filePath);
        if (!logFile_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            qWarning() << "Failed to open log file:" << filePath;
            return false;
        }

        logStream_.setDevice(&logFile_);

        initialized_ = true;

        // 写入初始化日志
        QString initMsg = QString("Log system initialized. Log file: %1").arg(filePath);
        writeToFile(QString("[%1] [INFO] %2")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
            .arg(initMsg));

        qDebug() << initMsg;

        return true;
    }

    void Logger::setLogLevel(LogLevel level) {
        QMutexLocker locker(&mutex_);
        currentLevel_ = level;
    }

    LogLevel Logger::logLevel() const {
        QMutexLocker locker(&mutex_);
        return currentLevel_;
    }

    void Logger::log(LogLevel level, const QString &message) {
        if (level < currentLevel_) {
            return;
        }

        QMutexLocker locker(&mutex_);

        if (!initialized_) {
            // 如果未初始化，尝试自动初始化
            if (!initialize()) {
                // 初始化失败，回退到 qDebug
                qDebug() << "[" << levelToString(level) << "]" << message;
                return;
            }
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        QString levelStr = levelToString(level);
        QString logLine = QString("[%1] [%2] [Thread:%3] %4")
                .arg(timestamp)
                .arg(levelStr)
                .arg(QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16))
                .arg(message);

        writeToFile(logLine);
    }

    void Logger::debug(const QString &message) {
        instance().log(LogLevel::Debug, message);
    }

    void Logger::info(const QString &message) {
        instance().log(LogLevel::Info, message);
    }

    void Logger::warning(const QString &message) {
        instance().log(LogLevel::Warning, message);
    }

    void Logger::error(const QString &message) {
        instance().log(LogLevel::Error, message);
    }

    void Logger::critical(const QString &message) {
        instance().log(LogLevel::Critical, message);
    }

    void Logger::shutdown() {
        QMutexLocker locker(&mutex_);

        if (initialized_) {
            writeToFile(QString("[%1] [INFO] Log system shutdown.")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")));

            logStream_.flush();
            logFile_.close();
            initialized_ = false;
        }
    }

    QString Logger::levelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARNING";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }

    QString Logger::getDefaultLogDir() const {
        // 使用与 ConfigManager 相同的配置目录，在其下创建 logs 子目录
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        return QDir(configDir).filePath("logs");
    }

    QString Logger::generateDefaultFileName() const {
        QString dateStr = QDate::currentDate().toString("yyyy-MM-dd");
        return QString("optikg_%1.log").arg(dateStr);
    }

    void Logger::writeToFile(const QString &logLine) {
        if (logStream_.device()) {
            logStream_ << logLine << "\n";
            logStream_.flush();
        }
    }

    // 静态消息处理器函数
    void Logger::qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        Q_UNUSED(context);

        LogLevel level = LogLevel::Debug;
        switch (type) {
            case QtDebugMsg:
                level = LogLevel::Debug;
                break;
            case QtInfoMsg:
                level = LogLevel::Info;
                break;
            case QtWarningMsg:
                level = LogLevel::Warning;
                break;
            case QtCriticalMsg:
                level = LogLevel::Critical;
                break;
            case QtFatalMsg:
                level = LogLevel::Critical;
                // 致命错误，记录后终止
                instance().log(level, QString("FATAL: %1").arg(msg));
                // 恢复默认处理器以便显示错误对话框
                restoreDefaultMessageHandler();
                // 调用默认处理器终止程序
                qFatal("%s", msg.toUtf8().constData());
                break;
        }

        // 输出到日志文件
        instance().log(level, msg);

        // 如果启用控制台输出，则同时输出到控制台
        if (s_enableConsoleOutput) {
            QTextStream out(stdout);
            switch (type) {
                case QtDebugMsg:
                case QtInfoMsg:
                    out << msg << "\n";
                    break;
                case QtWarningMsg:
                    out << "WARNING: " << msg << "\n";
                    break;
                case QtCriticalMsg:
                    out << "CRITICAL: " << msg << "\n";
                    break;
                case QtFatalMsg:
                    // 已处理
                    break;
            }
            out.flush();
        }
    }

    void Logger::installQtMessageHandler(bool enableConsoleOutput) {
        s_enableConsoleOutput = enableConsoleOutput;
        qInstallMessageHandler(qtMessageHandler);
    }

    void Logger::restoreDefaultMessageHandler() {
        qInstallMessageHandler(nullptr);
    }
} // namespace optikg
