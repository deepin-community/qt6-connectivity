/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -a leadvertisement1_p.h:leadvertisement1.cpp -c OrgBluezLEAdvertisement1Adaptor -i bluez5_helper_p.h org.bluez.LEAdvertisement1.xml
 *
 * qdbusxml2cpp is Copyright (C) 2022 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef LEADVERTISEMENT1_P_H
#define LEADVERTISEMENT1_P_H

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
 * Adaptor class for interface org.bluez.LEAdvertisement1
 */
class OrgBluezLEAdvertisement1Adaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.bluez.LEAdvertisement1")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.bluez.LEAdvertisement1\">\n"
"    <method name=\"Release\">\n"
"      <annotation value=\"true\" name=\"org.freedesktop.DBus.Method.NoReply\"/>\n"
"    </method>\n"
"    <property access=\"readwrite\" type=\"s\" name=\"Type\"/>\n"
"    <property access=\"readwrite\" type=\"as\" name=\"ServiceUUIDs\"/>\n"
"    <property access=\"readwrite\" type=\"a{qv}\" name=\"ManufacturerData\">\n"
"      <annotation value=\"ManufacturerDataList\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <property access=\"readwrite\" type=\"b\" name=\"Discoverable\"/>\n"
"    <property access=\"readwrite\" type=\"as\" name=\"Includes\"/>\n"
"    <property access=\"readwrite\" type=\"s\" name=\"LocalName\"/>\n"
"    <property access=\"readwrite\" type=\"u\" name=\"MinInterval\"/>\n"
"    <property access=\"readwrite\" type=\"u\" name=\"MaxInterval\"/>\n"
"  </interface>\n"
        "")
public:
    OrgBluezLEAdvertisement1Adaptor(QObject *parent);
    virtual ~OrgBluezLEAdvertisement1Adaptor();

public: // PROPERTIES
    Q_PROPERTY(bool Discoverable READ discoverable WRITE setDiscoverable)
    bool discoverable() const;
    void setDiscoverable(bool value);

    Q_PROPERTY(QStringList Includes READ includes WRITE setIncludes)
    QStringList includes() const;
    void setIncludes(const QStringList &value);

    Q_PROPERTY(QString LocalName READ localName WRITE setLocalName)
    QString localName() const;
    void setLocalName(const QString &value);

    Q_PROPERTY(ManufacturerDataList ManufacturerData READ manufacturerData WRITE setManufacturerData)
    ManufacturerDataList manufacturerData() const;
    void setManufacturerData(ManufacturerDataList value);

    Q_PROPERTY(uint MaxInterval READ maxInterval WRITE setMaxInterval)
    uint maxInterval() const;
    void setMaxInterval(uint value);

    Q_PROPERTY(uint MinInterval READ minInterval WRITE setMinInterval)
    uint minInterval() const;
    void setMinInterval(uint value);

    Q_PROPERTY(QStringList ServiceUUIDs READ serviceUUIDs WRITE setServiceUUIDs)
    QStringList serviceUUIDs() const;
    void setServiceUUIDs(const QStringList &value);

    Q_PROPERTY(QString Type READ type WRITE setType)
    QString type() const;
    void setType(const QString &value);

public Q_SLOTS: // METHODS
    Q_NOREPLY void Release();
Q_SIGNALS: // SIGNALS
};

#endif
