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
 
#ifndef MAVLINK_INTERFACE_H
#define MAVLINK_INTERFACE_H

#include <QObject>
#include <QSerialPort>
#include <QTcpSocket>
#include <QVariant>

#include <mavlink_types.h>

Q_DECLARE_METATYPE(mavlink_message_t)

namespace C {
const int SystemId = 1;
const int ComponentId = 1;
}

/**
 * @brief The abstraction to interface with MAVLink IMU via serial or TCP
 */
class MavlinkInterface : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)
    Q_PROPERTY(QString serialName READ serialName WRITE setSerialName)
    Q_PROPERTY(QString serialRate READ serialRate WRITE setSerialRate)
    Q_PROPERTY(QString tcpAddress READ tcpAddress WRITE setTcpAddress)
    Q_PROPERTY(quint16 tcpPort READ tcpPort WRITE setTcpPort)

public:
    enum Interface {
        SerialInterface = 0,
        TcpInterface
    };

    MavlinkInterface(QObject *parent = Q_NULLPTR);

    /**
     * @brief MavlinkInterface destructor
     *
     * On destruction closes the connection
     */
    virtual ~MavlinkInterface();

    /**
     * @brief Get the connection status of the MAVLink interface
     * @return true if connection is open, false otherwise
     */
    bool connected() const;

    /**
     * @brief Use serial as primary interface for all future actions
     */
    Q_INVOKABLE void setSerialInterface() { m_interface = SerialInterface; }
    /**
     * @brief Use TCP as primary interface for all future actions
     */
    Q_INVOKABLE void setTcpInterface() { m_interface = TcpInterface; }

    /**
     * @brief Get the regex for serial interface name
     * @return Regex for serial interface name
     */
    QString serialName() const { return m_serialName; }
    /**
     * @brief Set the primary serial interface name
     * @param serialName primary serial interface name
     */
    void setSerialName(const QString &serialName) { m_serialName = serialName; }
    /**
     * @brief Get the currently used serial interface name
     * @return Currently used serial interface name
     */
    QString usedSerialName() const { return m_usedSerialName; }

    /**
     * @brief Get currently used serial interface rate
     * @return Rate in bauds per second
     */
    QString serialRate() const { return m_serialRate; }
    /**
     * @brief Set serial interface rate to use in future configurations
     * @param serialRate string representation of rate in bauds per second
     */
    void setSerialRate(const QString &serialRate) { m_serialRate = serialRate; }

    /**
     * @brief Get IP address used for TCP communication with a MAVLink device
     * @return String representation of an IP address
     */
    QString tcpAddress() const { return m_tcpAddress; }
    /**
     * @brief Set IP address for TCP communication with a MAVLink device
     * @param tcpAddress string representation of an IP address
     */
    void setTcpAddress(const QString &tcpAddress) { m_tcpAddress = tcpAddress; }

    /**
     * @brief Get TCP port used for communication with a MAVLink device
     * @return TCP port
     */
    quint16 tcpPort() const { return m_tcpPort; }
    /**
     * @brief Set TCP port used for communication with a MAVLink device
     * @param tcpPort port to communicate with
     */
    void setTcpPort(const quint16 &tcpPort) { m_tcpPort = tcpPort; }

    /**
     * @brief Get available serial ports
     * @return List of available ports (represented by QStrings)
     */
    Q_INVOKABLE QVariantList availablePorts() const;

    /**
     * @brief Open serial or TCP connection
     * @return true on success, false otherwise
     */
    Q_INVOKABLE bool open();
    /**
     * @brief Close serial or TCP connection
     */
    Q_INVOKABLE void close();
    /**
     * @brief Open serial or TCP interface if there's no active connection
     * @return true if after the call there's a connection, false otherwise
     */
    Q_INVOKABLE bool tryOpen();
    /**
     * @brief Clear serial port buffers
     * @return true on success, false otherwise
     */
    Q_INVOKABLE bool clear();
    /**
     * @brief Try to use another serial interfaces from list specified by regex on startup
     *
     * If a serial interface was specified on startup by regex matching several interfaces
     * this function will pick next interface and try to use it for communication
     *
     * @return true if interfaces changed successfully and new one is open
     */
    bool tryAnotherSerialInterface();
    /**
     * @brief Send MAVLink message
     * @param message MAVLink message to send
     * @return true if the message was written to an IO buffer, false otherwise
     */
    bool sendMessage(const mavlink_message_t &message);

protected:
    /**
     * @brief Try to open the interface by timer in case of a disconnection
     * @param event unused QTimerEvent parameter
     */
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

signals:
    /**
     * @brief Signal emitted on interface open or close
     */
    void connectionChanged();

    /**
     * @brief Emitted on MAVLink packet receive
     * @param msg received MAVLink packet
     */
    void hasMessage(const mavlink_message_t &msg);

private slots:
    /**
     * @brief Get new MAVLink data from a serial interface
     */
    void getSerialData();

    /**
     * @brief Get new MAVLink data from a network interface
     */
    void getTcpData();

    /**
     * @brief Set timer for reconnection in case of a disconnection
     */
    void reconnect();

private:
    /**
     * @brief Parse incoming MAVLink data
     * @param data incoming raw MAVLink data to parse
     */
    void parseMavlink(const QByteArray &data);
    bool sendMessage() { return sendMessage(m_outMessage); }

    /**
     * @brief Get next serial interface by regex
     */
    void pickNextSerial();

private:
    /**
     * @brief Type of an interface to use
     */
    Interface m_interface = SerialInterface;
    /**
     * @brief Serial interface name, like /dev/ttyACM0 or COM5 (or regex)
     */
    QString m_serialName;
    /**
     * @brief Currently used serial interface name
     */
    QString m_usedSerialName;
    /**
     * @brief Serial interface rate in bauds per second
     */
    QString m_serialRate = "57600";
    /**
     * @brief IP address for TCP connection
     */
    QString m_tcpAddress = "127.0.0.1";
    /**
     * @brief TCP port for MAVLink connection by network
     */
    quint16 m_tcpPort = 5760;

    /**
     * @brief Serial port to use for MAVLink data exchange
     */
    QSerialPort m_serialPort;
    /**
     * @brief TCP socket to use for MAVLink data exchange
     */
    QTcpSocket m_tcpSocket;
    /**
     * @brief Pointer to m_serialPort or m_tcpSocket
     */
    QIODevice *m_io = Q_NULLPTR;
    /**
     * @brief Reconnection timer ID used in case of a disconnection
     */
    int m_timerId = -1;

    /**
     * @brief Outgoing MAVLink message to be sent
     */
    mavlink_message_t m_outMessage;
};

#endif // #ifndef MAVLINK_INTERFACE_H
