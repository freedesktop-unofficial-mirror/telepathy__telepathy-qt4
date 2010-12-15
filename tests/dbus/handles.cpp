#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/PendingHandles>
#include <TelepathyQt4/PendingVoid>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestHandles : public Test
{
    Q_OBJECT

public:
    TestHandles(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectConnReady(Tp::ConnectionStatus, Tp::ConnectionStatusReason);
    void expectConnInvalidated();
    void expectPendingHandlesFinished(Tp::PendingOperation*);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRequestAndRelease();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    TpTestsSimpleConnection *mConnService;
    ConnectionPtr mConn;
    ReferencedHandles mHandles;
};

void TestHandles::expectConnReady(Tp::ConnectionStatus newStatus,
        Tp::ConnectionStatusReason newStatusReason)
{
    switch (newStatus) {
    case ConnectionStatusDisconnected:
        qWarning() << "Disconnected";
        mLoop->exit(1);
        break;
    case ConnectionStatusConnecting:
        /* do nothing */
        break;
    case ConnectionStatusConnected:
        qDebug() << "Ready";
        mLoop->exit(0);
        break;
    default:
        qWarning().nospace() << "What sort of status is "
            << newStatus << "?!";
        mLoop->exit(2);
        break;
    }
}

void TestHandles::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestHandles::expectPendingHandlesFinished(PendingOperation *op)
{
    if (!op->isFinished()) {
        qWarning() << "unfinished";
        mLoop->exit(1);
        return;
    }

    if (op->isError()) {
        qWarning().nospace() << op->errorName()
            << ": " << op->errorMessage();
        mLoop->exit(2);
        return;
    }

    if (!op->isValid()) {
        qWarning() << "inconsistent results";
        mLoop->exit(3);
        return;
    }

    qDebug() << "finished";
    PendingHandles *pending = qobject_cast<PendingHandles*>(op);
    mHandles = pending->handles();
    mLoop->exit(0);
}

void TestHandles::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("handles");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = TP_TESTS_SIMPLE_CONNECTION(g_object_new(
            TP_TESTS_TYPE_SIMPLE_CONNECTION,
            "account", "me@example.com",
            "protocol", "simple",
            NULL));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "simple", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mConn = Connection::create(mConnName, mConnPath,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    QVERIFY(connect(mConn->lowlevel()->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);

    if (mConn->status() != ConnectionStatusConnected) {
        QVERIFY(connect(mConn.data(),
                        SIGNAL(statusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason)),
                        SLOT(expectConnReady(Tp::ConnectionStatus, Tp::ConnectionStatusReason))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(mConn.data(),
                           SIGNAL(statusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason)),
                           this,
                           SLOT(expectConnReady(Tp::ConnectionStatus, Tp::ConnectionStatusReason))));
        QCOMPARE(mConn->status(), ConnectionStatusConnected);
    }
}

void TestHandles::init()
{
    initImpl();
}

void TestHandles::testRequestAndRelease()
{
    // Test identifiers
    QStringList ids = QStringList() << QLatin1String("alice")
        << QLatin1String("bob") << QLatin1String("chris");

    // Request handles for the identifiers and wait for the request to process
    PendingHandles *pending = mConn->lowlevel()->requestHandles(Tp::HandleTypeContact, ids);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingHandlesFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(pending,
                       SIGNAL(finished(Tp::PendingOperation*)),
                       this,
                       SLOT(expectPendingHandlesFinished(Tp::PendingOperation*))));
    ReferencedHandles handles = mHandles;
    mHandles = ReferencedHandles();

    // Verify that the closure indicates correctly which names we requested
    QCOMPARE(pending->namesRequested(), ids);

    // Verify by directly poking the service that the handles correspond to the requested IDs
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);
    for (int i = 0; i < 3; i++) {
        uint handle = handles[i];
        QCOMPARE(QString::fromUtf8(tp_handle_inspect(serviceRepo, handle)), ids[i]);
    }

    // Save the handles to a non-referenced normal container
    Tp::UIntList saveHandles = handles.toList();

    // Start releasing the handles, RAII style, and complete the asynchronous process doing that
    handles = ReferencedHandles();
    mLoop->processEvents();
    processDBusQueue(mConn.data());
}

void TestHandles::cleanup()
{
    cleanupImpl();
}

void TestHandles::cleanupTestCase()
{
    if (mConn) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->lowlevel()->requestDisconnect(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn.data(),
                            SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
                            SLOT(expectConnInvalidated())));
            QCOMPARE(mLoop->exec(), 0);
        }
    }

    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestHandles)
#include "_gen/handles.cpp.moc.hpp"
