#include "ui/mainwindow.h"
#include "utils/logger.h"
#include "utils/stylemanager.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QDebug>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 初始化日志系统
    optikg::Logger::instance().initialize();
    optikg::Logger::installQtMessageHandler(true); // 不输出到控制台，只写入文件
    optikg::Logger::info("Application started.");
    qDebug("Debug message from qDebug");

    // 初始化样式管理器 - 加载现代专业版主题 (QSS)
    optikg::StyleManager::instance().initialize();

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "qt-try_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    optikg::MainWindow w;
    w.show();
    return a.exec();
}
