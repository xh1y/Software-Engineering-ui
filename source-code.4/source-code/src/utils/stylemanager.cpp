#include "stylemanager.h"

#include <QApplication>
#include <QFile>
#include <QDebug>

namespace optikg {

StyleManager& StyleManager::instance() {
    static StyleManager instance;
    return instance;
}

void StyleManager::initialize() {
    // 默认加载现代专业主题 (Modern Pro)
    if (!loadStyleSheetFromResource(":/styles/modern_pro.qss")) {
        // 如果资源加载失败，尝试从文件系统加载
        QStringList possiblePaths = {
            "resources/styles/modern_pro.qss",
            "./resources/styles/modern_pro.qss",
            "../resources/styles/modern_pro.qss"
        };
        
        for (const QString& path : possiblePaths) {
            if (loadStyleSheet(path)) {
                qDebug() << "Loaded stylesheet from:" << path;
                return;
            }
        }
        
        qWarning() << "Failed to load default stylesheet:" << lastError_;
    }
}

bool StyleManager::loadStyleSheet(const QString& filePath) {
    QFile file(filePath);
    if (!file.exists()) {
        lastError_ = QString("Style file does not exist: %1").arg(filePath);
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lastError_ = QString("Cannot open style file: %1").arg(file.errorString());
        return false;
    }

    QTextStream stream(&file);
    QString styleSheet = stream.readAll();
    file.close();

    applyStyleSheet(styleSheet);
    return true;
}

bool StyleManager::loadStyleSheetFromResource(const QString& resourcePath) {
    QFile file(resourcePath);
    if (!file.exists()) {
        lastError_ = QString("Resource file does not exist: %1").arg(resourcePath);
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lastError_ = QString("Cannot open resource file: %1").arg(file.errorString());
        return false;
    }

    QTextStream stream(&file);
    QString styleSheet = stream.readAll();
    file.close();

    applyStyleSheet(styleSheet);
    return true;
}

void StyleManager::applyStyleSheet(const QString& styleSheet) {
    currentStyleSheet_ = styleSheet;
    qApp->setStyleSheet(styleSheet);
    qDebug() << "Stylesheet applied, length:" << styleSheet.length() << "chars";
}

bool StyleManager::reloadCurrentTheme() {
    // 强制刷新当前样式
    qApp->setStyleSheet("");
    qApp->setStyleSheet(currentStyleSheet_);
    return true;
}

} // namespace optikg
