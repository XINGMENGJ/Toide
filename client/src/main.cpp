#include "app/main_window.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/toide/resources/app.ico")));

    MainWindow window;
    window.show();

    return QApplication::exec();
}
