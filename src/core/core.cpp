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

#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include <QDebug>

namespace C {
const char * const ApiHost = "http://127.0.0.1:8123";
const char * const ApiPath = "/api/v1";
}

const quint32 MAX_LOST_COUNTER = 5;

Core::Core(QObject *parent) :
    QObject(parent), m_heartbitCounter(10), m_lostCounter(0)
{
    m_mavlinkInterface = new MavlinkInterface(this);
    connect(m_mavlinkInterface, &MavlinkInterface::hasMessage,
            this, &Core::handleMessage);

    connect(&m_lifeTimer, &QTimer::timeout,
            this, &Core::lost);
}

Core::~Core()
{
}

void Core::handleMessage(const mavlink_message_t &msg)
{
    switch (msg.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: {
#ifdef DEBUG
        qDebug() << "HEARTBEAT";
#endif
        mavlink_heartbeat_t packet;
        mavlink_msg_heartbeat_decode(&msg, &packet);
        m_lostCounter = 0;
        m_lifeTimer.start(3000);
        if (!m_connected && (m_heartbitCounter == 0)) {
            m_connected = true;
            init();
        } else if (m_heartbitCounter > 0) {
            m_heartbitCounter--;
        }
        break;
    }
    case MAVLINK_MSG_ID_ATTITUDE: {
        mavlink_attitude_t packet;
        mavlink_msg_attitude_decode(&msg, &packet);
        sendAngles(packet.roll, packet.pitch, packet.yaw);
#ifdef DEBUG
        qDebug() << "MAVLINK_MSG_ID_ATTITUDE" <<
                    "Roll:" << packet.roll <<
                    "Pitch:" << packet.pitch <<
                    "Yaw:" << packet.yaw;
#endif
        break;
    }
    default:
#ifdef DEBUG
        static QSet<quint8> ids;
        ids << msg.msgid;
        QList<quint8> list = ids.toList();
        qSort(list);
        qDebug() << "IDs" << list;
#endif
        break;
    }
}

void Core::handleReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply);
    if (reply->error()) {
        qWarning().noquote() << tr("Warning: HTTP request failed ('%1').")
                                .arg(reply->request().url().toString());
    }
    reply->deleteLater();
}

void Core::init()
{
    requestDataStream(MAV_DATA_STREAM_EXTRA1, 10);
}

void Core::lost()
{
    qWarning().noquote() << tr("Warning: MAVLink connection lost.");

    if (m_lostCounter >= MAX_LOST_COUNTER) {
        qWarning().noquote() << tr("Warning: Serious connection loss. "
                                   "Trying to reconnect.");
        bool reconnectResult = m_mavlinkInterface->tryAnotherSerialInterface();
        m_connected = false;
        m_heartbitCounter = 10;
        qWarning().noquote() << tr("Warning: New interface is: ") <<  m_mavlinkInterface->usedSerialName();
        qWarning().noquote() << tr("Warning: Serial reconnect ")
                             << tr(reconnectResult ? "OK" : "not OK");
    } else {
        m_lostCounter++;
    }
}

void Core::requestDataStream(MAV_DATA_STREAM stream, quint16 rate)
{
    mavlink_request_data_stream_t packet;
    memset(&packet, 0, sizeof(packet));
    packet.req_stream_id = stream;
    packet.req_message_rate = rate;
    packet.start_stop = 1; // start

    mavlink_message_t message;
    mavlink_msg_request_data_stream_encode(C::SystemId,
                                           MAV_COMP_ID_SYSTEM_CONTROL,
                                           &message,
                                           &packet);

    m_mavlinkInterface->sendMessage(message);
}

void Core::sendAngle(const QString &angle, float value)
{
    QUrl requestUrl(QString("%1%2/imu_%3/%4")
                    .arg(C::ApiHost)
                    .arg(C::ApiPath)
                    .arg(angle)
                    .arg(value));
    QNetworkReply *reply =
            m_net.post(QNetworkRequest(requestUrl), QByteArray());
    connect(reply, &QNetworkReply::finished, this, &Core::handleReply);
}

void Core::sendAngles(float roll, float pitch, float yaw) {
    QUrl requestUrl(QString("%1%2/imu_all/%3:%4:%5")
                    .arg(C::ApiHost)
                    .arg(C::ApiPath)
                    .arg(roll)
                    .arg(pitch)
                    .arg(yaw));
    QNetworkReply *reply =
            m_net.post(QNetworkRequest(requestUrl), QByteArray());
    connect(reply, &QNetworkReply::finished, this, &Core::handleReply);
}

bool Core::start()
{
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::NoProxy);
    m_net.setProxy(proxy);

    if (!m_mavlinkInterface->open()) {
        qCritical().noquote() << tr("Error: Failed to open MAVLink device.");
        return false;
    }
    m_mavlinkInterface->clear();
    m_lifeTimer.start(6000);
    return true;
}


void Core::stop()
{
    m_mavlinkInterface->close();
}
