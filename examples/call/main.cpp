#include <glib-object.h>
#include <gst/gst.h>

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QDebug>
#include <QtGui>

#include "call-window.h"

int main(int argc, char **argv)
{
    g_type_init();
    gst_init(&argc, &argv);

    QApplication app(argc, argv);

    if (argc < 3) {
        qDebug() << "usage: call username password";
        return 1;
    }

    Telepathy::registerTypes();
    Telepathy::enableDebug(true);
    Telepathy::enableWarnings(true);

    CallWindow w(argv[1], argv[2]);
    w.show();

    return app.exec();
}
