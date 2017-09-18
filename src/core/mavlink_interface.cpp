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
 
#include "mavlink_interface.h"

#include <QSerialPortInfo>

#include <QDebug>

#include <common/mavlink.h>

MavlinkInterface::MavlinkInterface(QObject *parent) : QObject(parent)
{
    connect(&m_serialPort, &QSerialPort::readyRead,
            this, &MavlinkInterface::getSerialData);

    connect(&m_tcpSocket, &QTcpSocket::readyRead,
            this, &MavlinkInterface::getTcpData);

    memset(&m_outMessage, 0, sizeof(m_outMessage));
}

MavlinkInterface::~MavlinkInterface()
{
    close();
}

QVariantList MavlinkInterface::availablePorts() const
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    QVariantList result;
    foreach (QSerialPortInfo port, ports) {
        result << port.portName();
    }
    qSort(result);
    return result;
}

bool MavlinkInterface::clear()
{
    return m_serialPort.clear();
}

void MavlinkInterface::close()
{
    disconnect(&m_tcpSocket, &QTcpSocket::disconnected,
            this, &MavlinkInterface::reconnect);
    if (m_io && m_io->isOpen()) {
        m_io->close();
        m_io = Q_NULLPTR;
        emit connectionChanged();
    }
}

bool MavlinkInterface::connected() const
{
    switch (m_interface) {
    case SerialInterface:
        return m_serialPort.isOpen();
        break;

    case TcpInterface:
        return (m_tcpSocket.state()== QTcpSocket::ConnectedState);
        break;

    default:
        break;
    }
    return false;
}

void MavlinkInterface::getSerialData()
{
    parseMavlink(m_serialPort.readAll());
}

void MavlinkInterface::getTcpData()
{
    parseMavlink(m_tcpSocket.readAll());
}

void MavlinkInterface::pickNextSerial() {
    QList<QSerialPortInfo> portsInfo = QSerialPortInfo::availablePorts();
    QList<QSerialPortInfo> matches;
    for (QSerialPortInfo portInfo : portsInfo) {
        QRegExp portRegExp(m_serialName);
        if (portRegExp.exactMatch(portInfo.portName()) ||
                portRegExp.exactMatch(portInfo.systemLocation())) {
            matches.append(portInfo);
        }
    }

    if (matches.size() > 0 && m_usedSerialName.size() == 0) {
        m_usedSerialName = matches.first().systemLocation();
    } else if (matches.size() > 0) {
        bool wasSet = false;

        QList<QSerialPortInfo>::iterator i;
        for (i = matches.begin(); !wasSet && i != matches.end(); i++) {
            if (i->systemLocation().compare(m_usedSerialName) > 0) {
                m_usedSerialName = i->systemLocation();
                wasSet = true;
            }
        }

        if (!wasSet) {
            m_usedSerialName = matches.first().systemLocation();
        }
    } else {
        m_usedSerialName = m_serialName;
    }
}

bool MavlinkInterface::open()
{
    bool result = false;
    switch (m_interface) {
    case SerialInterface:
        if (m_serialPort.isOpen()) {
            close();
        }
        pickNextSerial();

        m_serialPort.setPortName(m_usedSerialName);
        m_serialPort.setBaudRate(m_serialRate.toInt());
        m_serialPort.setDataBits(QSerialPort::Data8);
        m_serialPort.setStopBits(QSerialPort::OneStop);
        m_serialPort.setFlowControl(QSerialPort::NoFlowControl);
        m_serialPort.setParity(QSerialPort::NoParity);
        m_io = &m_serialPort;
        result = m_serialPort.open(QSerialPort::ReadWrite);
        if (m_serialPort.isOpen()) {
            emit connectionChanged();
        }
        break;

    case TcpInterface:
        m_tcpSocket.abort();
        m_tcpSocket.connectToHost(m_tcpAddress, m_tcpPort);
        m_io = &m_tcpSocket;
        result = m_tcpSocket.waitForConnected(1000);
        if (m_tcpSocket.isOpen()) {
            connect(&m_tcpSocket, &QTcpSocket::disconnected,
                    this, &MavlinkInterface::reconnect);
            emit connectionChanged();
        }
        break;
    }

    return result;
}

void MavlinkInterface::parseMavlink(const QByteArray &data)
{
    enum State {
        IdleState = 0,
        HeaderState,
        PayloadState,
        ChecksumState
    };
    static State state = IdleState;
    static size_t offset = 0;
    static mavlink_message_t msg;
    char *header = reinterpret_cast<char *>(&msg.magic);
    char *payload = reinterpret_cast<char *>(msg.payload64);
    char *checksum = reinterpret_cast<char *>(&msg.checksum);

    for (int i = 0; i < data.size(); ++i) {
        const char c = data.at(i);
        switch (state) {
        case IdleState:
            if (c == static_cast<char>(0xFE)) {
                header[offset++] = c;
                state = HeaderState;
            }
            break;
        case HeaderState:
            header[offset++] = c;
            if (offset >= MAVLINK_NUM_HEADER_BYTES) {
                offset = 0;
                state = PayloadState;
                // TODO: Check msg.seq
            }
            break;
        case PayloadState:
            payload[offset++] = c;
            if (offset >= msg.len) {
                offset = 0;
                state = ChecksumState;
            }
            break;
        case ChecksumState:
            checksum[offset++] = c;
            if (offset >= MAVLINK_NUM_CHECKSUM_BYTES) {
                offset = 0;
                state = IdleState;

                // TODO: Add checksum validation
                emit hasMessage(msg);
            }
            break;
        }
    }
}

void MavlinkInterface::reconnect()
{
    close();
    if (m_timerId == -1) {
        m_timerId = startTimer(1000);
    }
}

bool MavlinkInterface::tryAnotherSerialInterface() {
    bool result = false;
    if (m_interface == SerialInterface) {
        close();
        result = open();
    }
    return result;
}

bool MavlinkInterface::sendMessage(const mavlink_message_t &message)
{
    if(!message.len) {
        return false;
    }
    if (!tryOpen()) {
        return false;
    }
    Q_ASSERT(m_io != Q_NULLPTR);

    QByteArray buffer;
    buffer.reserve(message.len +
                   MAVLINK_NUM_HEADER_BYTES + MAVLINK_NUM_CHECKSUM_BYTES);
    buffer.append(reinterpret_cast<const char *>(&message.magic),
                  MAVLINK_NUM_HEADER_BYTES);
    buffer.append(reinterpret_cast<const char *>(message.payload64),
                  message.len);
    buffer.append(reinterpret_cast<const char *>(&message.checksum),
                  sizeof(message.checksum));
    m_io->write(buffer);

    return true;
}

void MavlinkInterface::timerEvent(QTimerEvent * /*event*/)
{
    if (tryOpen()) {
        killTimer(m_timerId);
        m_timerId = -1;
    }
}

bool MavlinkInterface::tryOpen()
{
    if (connected()) {
        return true;
    }
    return open();
}
