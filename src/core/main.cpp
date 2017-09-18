/*
 * Copyright (c) 2017, Smart Projects Holdings Ltd
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Smart Projects Holdings Ltd nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SMART PROJECTS HOLDINGS LTD BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#include "core.h"
#include "mavlink_interface.h"

#include <app_version.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

#include <vector>
#include <signal.h>

void quit(int result = 0);

void quit(int /*result*/)
{
    QCoreApplication::quit();
}

static QString tr(const char *key)
{
    return QCoreApplication::translate("main", key);
}

int main(int argc, char *argv[])
{
    std::vector<char> name(strlen(C::AppName) + 1);
    strcpy(name.data(), C::AppName);
    argv[0] = name.data();

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(C::AppName);
    QCoreApplication::setApplicationVersion(tr("v%1 build %2").arg(C::AppVersion).arg(APP_VERSION_BUILD));

    QCommandLineParser parser;
    parser.setApplicationDescription(tr("Attitude angles feeder"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption serialOption(QStringList() << "s" << "serial",
                                    tr("MAVLink serial device (e.g. '/dev/ttyACM0' or 'COM10')."),
                                    tr("serial"));
    parser.addOption(serialOption);

    QCommandLineOption networkOption(QStringList() << "n" << "network",
                                     tr("MAVLink network device address (e.g. localhost or 127.0.0.1)."),
                                     tr("network"), "127.0.0.1");
    parser.addOption(networkOption);
    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  tr("MAVLink network device port"),
                                  tr("port"), "5760");
    parser.addOption(portOption);

    parser.process(app);

    auto core = new Core(&app);
    // One and only one MAVLink device type must be specified!
    if (!(parser.isSet(serialOption) ^
            (parser.isSet(networkOption) && parser.isSet(portOption)))) {
        qCritical().noquote() << tr("One and only one MAVLink device type must be specified!") << endl;
        parser.showHelp(1);
        Q_UNREACHABLE();
    } else if (parser.isSet(serialOption)) {
        QString serialName = parser.value(serialOption);

        core->mavlinkInterface()->setSerialName(serialName);
        core->mavlinkInterface()->setSerialInterface();
    } else if (parser.isSet(networkOption) && parser.isSet(portOption)) {
        QString networkAddr = parser.value(networkOption);
        quint16 port = parser.value(portOption).toInt();

        core->mavlinkInterface()->setTcpAddress(networkAddr);
        core->mavlinkInterface()->setTcpPort(port);
        core->mavlinkInterface()->setTcpInterface();
    }

    signal(SIGINT, quit);

    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     core, &Core::stop);

    if (!core->start()) {
        core->stop();
        qCritical().noquote() << tr("%1 failed to start.").arg(C::AppName);
        return 1;
    }

    qInfo().noquote() << tr("%1 started.").arg(C::AppName);

    return app.exec();
}
