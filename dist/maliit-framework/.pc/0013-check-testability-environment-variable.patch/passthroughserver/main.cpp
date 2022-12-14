/* * This file is part of Maliit framework *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 *
 * Contact: maliit-discuss@lists.maliit.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */

#include <QtGlobal>

#include "connectionfactory.h"
#include "mimserver.h"
#include "mimserveroptions.h"
#ifndef NOXCB
#include "xcbplatform.h"
#endif
#ifdef HAVE_WAYLAND
#include "waylandplatform.h"
#endif // HAVE_WAYLAND
#include "unknownplatform.h"

#include <QGuiApplication>
#include <QtDebug>

#include <qpa/qplatformnativeinterface.h>

namespace {

void disableMInputContextPlugin()
{
    // none is a special value for QT_IM_MODULE, which disables loading of any
    // input method module in Qt 5.
    setenv("QT_IM_MODULE", "none", true);
}

bool isDebugEnabled()
{
    static int debugEnabled = -1;

    if (debugEnabled == -1) {
        QByteArray debugEnvVar = qgetenv("MALIIT_DEBUG");
        if (!debugEnvVar.isEmpty() && debugEnvVar != "0") {
            debugEnabled = 1;
        } else {
            debugEnabled = 0;
        }
    }

    return debugEnabled == 1;
}

void outputMessagesToStdErr(QtMsgType type,
                            const QMessageLogContext &context,
                            const QString &msg)
{
    Q_UNUSED(context);
    QByteArray utf_text(msg.toUtf8());
    const char *raw(utf_text.constData());

    switch (type) {
    case QtDebugMsg:
        if (isDebugEnabled()) {
            fprintf(stderr, "DEBUG: %s\n", raw);
        }
        break;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    case QtInfoMsg:
        fprintf(stderr, "INFO: %s\n", raw);
        break;
#endif
    case QtWarningMsg:
        fprintf(stderr, "WARNING: %s\n", raw);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "CRITICAL: %s\n", raw);
        break;
    case QtFatalMsg:
        fprintf(stderr, "FATAL: %s\n", raw);
        abort();
    }
}

QSharedPointer<MInputContextConnection> createConnection(const MImServerConnectionOptions &options)
{
#ifdef HAVE_WAYLAND
    if (QGuiApplication::platformName().startsWith("wayland")) {
        return QSharedPointer<MInputContextConnection>(Maliit::createWestonIMProtocolConnection());
    } else
#endif
    if (options.overriddenAddress.isEmpty()) {
        return QSharedPointer<MInputContextConnection>(Maliit::DBus::createInputContextConnectionWithDynamicAddress());
    } else {
        return QSharedPointer<MInputContextConnection>(Maliit::DBus::createInputContextConnectionWithFixedAddress(options.overriddenAddress,
                                                                                                                  options.allowAnonymous));
    }
}

QSharedPointer<Maliit::AbstractPlatform> createPlatform()
{
#ifdef HAVE_WAYLAND
    if (QGuiApplication::platformName().startsWith("wayland")) {
        return QSharedPointer<Maliit::AbstractPlatform>(new Maliit::WaylandPlatform);
    } else
#endif
#ifndef NOXCB
    if (QGuiApplication::platformName() == "xcb") {
        return QSharedPointer<Maliit::AbstractPlatform>(new Maliit::XCBPlatform);
    } else {
#else
    {
#endif
        return QSharedPointer<Maliit::AbstractPlatform>(new Maliit::UnknownPlatform);
    }
}

} // unnamed namespace

int main(int argc, char **argv)
{
    qInstallMessageHandler(outputMessagesToStdErr);

    // QT_IM_MODULE, MApplication and QtMaemo5Style all try to load
    // MInputContext, which is fine for the application. For the passthrough
    // server itself, we absolutely need to prevent that.
    disableMInputContextPlugin();

    MImServerCommonOptions serverCommonOptions;
    MImServerConnectionOptions connectionOptions;

    const bool allRecognized = parseCommandLine(argc, argv);
    if (serverCommonOptions.showHelp) {
        printHelpMessage();
        return 1;
    } else if (not allRecognized) {
        printHelpMessage();
    }

    QGuiApplication app(argc, argv);

    // The testability driver is only loaded by QApplication but not by QGuiApplication.
    // However, QApplication depends on QWidget which would add some unneeded overhead => Let's load the testability driver on our own.
    if (app.arguments().contains(QLatin1String("-testability"))) {
      QLibrary testLib(QLatin1String("qttestability"));
      if (testLib.load()) {
	typedef void (*TasInitialize)(void);
	TasInitialize initFunction = (TasInitialize)testLib.resolve("qt_testability_init");
	if (initFunction) {
	  initFunction();
	} else {
	  qCritical("Library qttestability resolve failed!");
	}
      } else {
	qCritical("Library qttestability load failed!");
      }
    }

    // Input Context Connection
    QSharedPointer<MInputContextConnection> icConnection(createConnection(connectionOptions));

    QSharedPointer<Maliit::AbstractPlatform> platform(createPlatform());

    // The actual server
    MImServer::configureSettings(MImServer::PersistentSettings);
    MImServer imServer(icConnection, platform);
    Q_UNUSED(imServer);

    QPlatformNativeInterface* nativeInterface = QGuiApplication::platformNativeInterface();
    nativeInterface->setProperty("UbuntuSessionType", 1);   // SYSTEM_SESSION_TYPE
    nativeInterface->setProperty("ubuntuSessionType", 1);   // SYSTEM_SESSION_TYPE
    nativeInterface->setProperty("session", 1);   // SYSTEM_SESSION_TYPE

    return app.exec();
}
