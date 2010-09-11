/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _TelepathyQt4_client_registrar_internal_h_HEADER_GUARD_
#define _TelepathyQt4_client_registrar_internal_h_HEADER_GUARD_

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

#include <TelepathyQt4/AbstractClientHandler>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Types>

namespace Tp
{

class PendingOperation;

class TELEPATHY_QT4_NO_EXPORT ClientAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Client")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Client\" >\n"
"    <property name=\"Interfaces\" type=\"as\" access=\"read\" />\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QStringList Interfaces READ Interfaces)

public:
    ClientAdaptor(ClientRegistrar *registrar, const QStringList &interfaces, QObject *parent);
    virtual ~ClientAdaptor();

    inline const ClientRegistrar *registrar() const
    {
        return mRegistrar;
    }

public: // Properties
    inline QStringList Interfaces() const
    {
        return mInterfaces;
    }

private:
    ClientRegistrar *mRegistrar;
    QStringList mInterfaces;
};

class TELEPATHY_QT4_NO_EXPORT ClientObserverAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Client.Observer")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Client.Observer\" >\n"
"    <property name=\"ObserverChannelFilter\" type=\"aa{sv}\" access=\"read\" />\n"
"    <property name=\"Recover\" type=\"b\" access=\"read\" />\n"
"    <method name=\"ObserveChannels\" >\n"
"      <arg name=\"Account\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Connection\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Channels\" type=\"a(oa{sv})\" direction=\"in\" />\n"
"      <arg name=\"Dispatch_Operation\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Requests_Satisfied\" type=\"ao\" direction=\"in\" />\n"
"      <arg name=\"Observer_Info\" type=\"a{sv}\" direction=\"in\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(Tp::ChannelClassList ObserverChannelFilter READ ObserverChannelFilter)

public:
    ClientObserverAdaptor(ClientRegistrar *registrar,
            AbstractClientObserver *client,
            QObject *parent);
    virtual ~ClientObserverAdaptor();

    inline const ClientRegistrar *registrar() const
    {
        return mRegistrar;
    }

public: // Properties
    inline Tp::ChannelClassList ObserverChannelFilter() const
    {
        return mClient->observerChannelFilter();
    }

    inline bool Recover() const
    {
        return mClient->shouldRecover();
    }

public Q_SLOTS: // Methods
    void ObserveChannels(const QDBusObjectPath &account,
            const QDBusObjectPath &connection,
            const Tp::ChannelDetailsList &channels,
            const QDBusObjectPath &dispatchOperation,
            const Tp::ObjectPathList &requestsSatisfied,
            const QVariantMap &observerInfo,
            const QDBusMessage &message);

private Q_SLOTS:
    void onReadyOpFinished(Tp::PendingOperation *);

private:
    struct InvocationData : RefCounted
    {
        InvocationData() : readyOp(0) {}

        PendingOperation *readyOp;
        QString error, message;

        MethodInvocationContextPtr<> ctx;
        AccountPtr acc;
        ConnectionPtr conn;
        QList<ChannelPtr> chans;
        ChannelDispatchOperationPtr dispatchOp;
        QList<ChannelRequestPtr> chanReqs;
        QVariantMap observerInfo;
    };
    QLinkedList<SharedPtr<InvocationData> > invocations;

    ClientRegistrar *mRegistrar;
    QDBusConnection mBus;
    AbstractClientObserver *mClient;
};

class TELEPATHY_QT4_NO_EXPORT ClientApproverAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Client.Approver")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Client.Approver\" >\n"
"    <property name=\"ApproverChannelFilter\" type=\"aa{sv}\" access=\"read\" />\n"
"    <method name=\"AddDispatchOperation\" >\n"
"      <arg name=\"Channels\" type=\"a(oa{sv})\" direction=\"in\" />\n"
"      <arg name=\"Dispatch_Operation\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Properties\" type=\"a{sv}\" direction=\"in\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(Tp::ChannelClassList ApproverChannelFilter READ ApproverChannelFilter)

public:
    ClientApproverAdaptor(ClientRegistrar *registrar,
            AbstractClientApprover *client,
            QObject *parent);
    virtual ~ClientApproverAdaptor();

    inline const ClientRegistrar *registrar() const
    {
        return mRegistrar;
    }

public: // Properties
    inline Tp::ChannelClassList ApproverChannelFilter() const
    {
        return mClient->approverChannelFilter();
    }

public Q_SLOTS: // Methods
    void AddDispatchOperation(const Tp::ChannelDetailsList &channels,
            const QDBusObjectPath &dispatchOperation,
            const QVariantMap &properties,
            const QDBusMessage &message);

private:
    ClientRegistrar *mRegistrar;
    QDBusConnection mBus;
    AbstractClientApprover *mClient;
};

class TELEPATHY_QT4_NO_EXPORT ClientHandlerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Client.Handler")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Client.Handler\" >\n"
"    <property name=\"HandlerChannelFilter\" type=\"aa{sv}\" access=\"read\" />\n"
"    <property name=\"BypassApproval\" type=\"b\" access=\"read\" />\n"
"    <property name=\"Capabilities\" type=\"as\" access=\"read\" />\n"
"    <property name=\"HandledChannels\" type=\"ao\" access=\"read\" />\n"
"    <method name=\"HandleChannels\" >\n"
"      <arg name=\"Account\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Connection\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Channels\" type=\"a(oa{sv})\" direction=\"in\" />\n"
"      <arg name=\"Requests_Satisfied\" type=\"ao\" direction=\"in\" />\n"
"      <arg name=\"User_Action_Time\" type=\"t\" direction=\"in\" />\n"
"      <arg name=\"Handler_Info\" type=\"a{sv}\" direction=\"in\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(Tp::ChannelClassList HandlerChannelFilter READ HandlerChannelFilter)
    Q_PROPERTY(bool BypassApproval READ BypassApproval)
    Q_PROPERTY(QStringList Capabilities READ Capabilities)
    Q_PROPERTY(Tp::ObjectPathList HandledChannels READ HandledChannels)

public:
    ClientHandlerAdaptor(ClientRegistrar *registrar,
            AbstractClientHandler *client,
            QObject *parent);
    virtual ~ClientHandlerAdaptor();

    inline const ClientRegistrar *registrar() const
    {
        return mRegistrar;
    }

public: // Properties
    inline Tp::ChannelClassList HandlerChannelFilter() const
    {
        return mClient->handlerChannelFilter();
    }

    inline bool BypassApproval() const
    {
        return mClient->bypassApproval();
    }

    inline QStringList Capabilities() const
    {
        return mClient->capabilities();
    }

    inline Tp::ObjectPathList HandledChannels() const
    {
        // mAdaptorsForConnection includes this, so no need to start the set
        // with this->mHandledChannels
        QList<ClientHandlerAdaptor*> adaptors(mAdaptorsForConnection.value(
                    qMakePair(mBus.name(), mBus.baseService())));
        QSet<ChannelPtr> handledChannels;
        foreach (ClientHandlerAdaptor *handlerAdaptor, adaptors) {
            handledChannels.unite(handlerAdaptor->mHandledChannels);
        }

        Tp::ObjectPathList ret;
        foreach (const ChannelPtr &channel, handledChannels) {
            ret.append(QDBusObjectPath(channel->objectPath()));
        }
        return ret;
    }

public Q_SLOTS: // Methods
    void HandleChannels(const QDBusObjectPath &account,
            const QDBusObjectPath &connection,
            const Tp::ChannelDetailsList &channels,
            const Tp::ObjectPathList &requestsSatisfied,
            qulonglong userActionTime,
            const QVariantMap &handlerInfo,
            const QDBusMessage &message);

private Q_SLOTS:
    void onChannelInvalidated(Tp::DBusProxy *proxy);

private:
    static void onContextFinished(const MethodInvocationContextPtr<> &context,
            const QList<ChannelPtr> &channels, ClientHandlerAdaptor *self);

    ClientRegistrar *mRegistrar;
    QDBusConnection mBus;
    AbstractClientHandler *mClient;
    QSet<ChannelPtr> mHandledChannels;

    static QHash<QPair<QString, QString>, QList<ClientHandlerAdaptor *> > mAdaptorsForConnection;
};

class TELEPATHY_QT4_NO_EXPORT ClientHandlerRequestsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Client.Interface.Requests")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Client.Interface.Requests\" >\n"
"    <method name=\"AddRequest\" >\n"
"      <arg name=\"Request\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Properties\" type=\"a{sv}\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"RemoveRequest\" >\n"
"      <arg name=\"Request\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Error\" type=\"s\" direction=\"in\" />\n"
"      <arg name=\"Message\" type=\"s\" direction=\"in\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

public:
    ClientHandlerRequestsAdaptor(ClientRegistrar *registrar,
            AbstractClientHandler *client,
            QObject *parent);
    virtual ~ClientHandlerRequestsAdaptor();

    inline const ClientRegistrar *registrar() const
    {
        return mRegistrar;
    }

public Q_SLOTS: // Methods
    void AddRequest(const QDBusObjectPath &request,
            const QVariantMap &requestProperties,
            const QDBusMessage &message);
    void RemoveRequest(const QDBusObjectPath &request,
            const QString &errorName, const QString &errorMessage,
            const QDBusMessage &message);

private:
    ClientRegistrar *mRegistrar;
    QDBusConnection mBus;
    AbstractClientHandler *mClient;
};

} // Tp

Q_DECLARE_METATYPE(Tp::ClientAdaptor*)
Q_DECLARE_METATYPE(Tp::ClientApproverAdaptor*)
Q_DECLARE_METATYPE(Tp::ClientHandlerAdaptor*)
Q_DECLARE_METATYPE(Tp::ClientHandlerRequestsAdaptor*)
Q_DECLARE_METATYPE(Tp::ClientObserverAdaptor*)

#endif
