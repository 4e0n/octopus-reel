#include <QApplication>
#include "patchmainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    PatchMainWindow w;
    w.show();

    return app.exec();
}
