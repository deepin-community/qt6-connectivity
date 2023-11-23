/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -a objectmanageradaptor_p.h:objectmanageradaptor.cpp -c OrgFreedesktopDBusObjectManagerAdaptor -i bluez5_helper_p.h org.freedesktop.dbus.objectmanager.xml
 *
 * qdbusxml2cpp is Copyright (C) 2022 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef OBJECTMANAGERADAPTOR_P_H
#define OBJECTMANAGERADAPTOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include "bluez5_helper_p.h"
#include <QtCore/qcontainerfwd.h>

/*
 * Adaptor class for interface org.freedesktop.DBus.ObjectManager
 */
class OrgFreedesktopDBusObjectManagerAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.DBus.ObjectManager")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.DBus.ObjectManager\">\n"
"    <method name=\"GetManagedObjects\">\n"
"      <arg direction=\"out\" type=\"a{oa{sa{sv}}}\" name=\"object_paths_interfaces_and_properties\"/>\n"
"      <annotation value=\"ManagedObjectList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"    </method>\n"
"    <signal name=\"InterfacesAdded\">\n"
"      <arg type=\"o\" name=\"object_path\"/>\n"
"      <arg type=\"a{sa{sv}}\" name=\"interfaces_and_properties\"/>\n"
"      <annotation value=\"InterfaceList\" name=\"org.qtproject.QtDBus.QtTypeName.Out1\"/>\n"
"    </signal>\n"
"    <signal name=\"InterfacesRemoved\">\n"
"      <arg type=\"o\" name=\"object_path\"/>\n"
"      <arg type=\"as\" name=\"interfaces\"/>\n"
"    </signal>\n"
"  </interface>\n"
        "")
public:
    OrgFreedesktopDBusObjectManagerAdaptor(QObject *parent);
    virtual ~OrgFreedesktopDBusObjectManagerAdaptor();

public: // PROPERTIES
public Q_SLOTS: // METHODS
    ManagedObjectList GetManagedObjects();
Q_SIGNALS: // SIGNALS
    void InterfacesAdded(const QDBusObjectPath &object_path, InterfaceList interfaces_and_properties);
    void InterfacesRemoved(const QDBusObjectPath &object_path, const QStringList &interfaces);
};

#endif
