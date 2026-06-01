#include "src/ui/mainwindow.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(QStringLiteral("FoxBender"));

    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale::system(),
                          QStringLiteral("qtbase"),
                          QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        a.installTranslator(&qtTranslator);
    }

    QTranslator appTranslator;
    const QString appDir = QApplication::applicationDirPath();
    const QStringList translationDirs = {
        appDir + QStringLiteral("/i18n"),
        appDir + QStringLiteral("/../i18n")
    };
    for (const QString &dir : translationDirs) {
        if (appTranslator.load(QLocale::system(),
                               QStringLiteral("FoxBender"),
                               QStringLiteral("_"),
                               dir)) {
            a.installTranslator(&appTranslator);
            break;
        }
    }

    MainWindow w;
    w.show();
    return QApplication::exec();
}
