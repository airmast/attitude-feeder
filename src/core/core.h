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
 
/**
 * @file core.h
 * @brief File contains a declaration of the primary application class - Core
 */

#ifndef CORE_H
#define CORE_H

#include <QObject>

#include <QNetworkAccessManager>
#include <QTimer>

#include <common/mavlink.h>

class MavlinkInterface;

/**
 * @brief A core application class to interface betveen MAVLink and server
 *
 * This is a core application class which controls the MAVLink interface,
 * receives data from an IMU device, dispatches useful messages and transmits
 * them to a server.
 *
 * @see MavlinkInterface
 */
class Core : public QObject
{
    Q_OBJECT
public:
    Core(QObject *parent = Q_NULLPTR);
    virtual ~Core();

    /**
     * @brief Start interfacing operations
     *
     * Starts all tasks performed by Core: opens MAVLink interface,
     * clears internal buffers and starts life timer to monitor loss of
     * connection (lack of HEARTBIT messages).
     *
     * @return true on success, false otherwise
     */
    bool start();

    /**
     * @brief Closes the MAVLink interface
     */
    void stop();

    /**
     * @brief Getter for the MAVLink interface used by Core
     * @return main MAVLink interface
     */
    MavlinkInterface *mavlinkInterface() const { return m_mavlinkInterface; }

    /**
     * @brief Send MAVLink packet to request a specific data stream
     * @param stream MAVLink data stream to use
     * @param rate requested message rate
     */
    void requestDataStream(MAV_DATA_STREAM stream, quint16 rate);

public slots:
    /**
     * @brief Handle incoming MAVLink message from the interface
     *
     * Handles incoming MAVLink messages. Only HEARTBIT and ATTITUDE messages
     * are really handled. All other messages are being just passed by.
     *
     * @param msg MAVLink message to handle
     */
    void handleMessage(const mavlink_message_t &msg);

private slots:
    /**
     * @brief Handle timer event if HEARTBITs don't come for too long
     */
    void lost();

    /**
     * @brief Handle HTTP reply
     */
    void handleReply();

private:

    /**
     * @brief Initalize the MAVLink data stream as a MAV_DATA_STREAM_EXTRA1
     */
    void init();

    /**
     * @brief Send gyroscope angles to the server by HTTP
     * @param roll gyroscope roll (rad)
     * @param pitch gyroscope pitch (rad)
     * @param yaw gyroscope yaw (rad)
     */
    void sendAngles(float roll, float pitch, float yaw);

    /**
     * @brief Send gyroscope angle to the server by HTTP
     * @param angle name of the angle we send ("roll", "pitch" or "yaw")
     * @param value value of the angle in radians
     */
    void sendAngle(const QString &angle, float value);

    /**
     * @brief Send gyroscope roll to the server
     * @param roll value of roll in radians
     */
    void sendRoll(float roll);

    /**
     * @brief Send gyroscope pitch to the server
     * @param pitch value of pitch in radians
     */
    void sendPitch(float pitch);

    /**
     * @brief Send gyroscope yaw to the server
     * @param yaw value of yaw in radians
     */
    void sendYaw(float yaw);

private:

    /**
     * @brief Whether we have initialized the MAVLink data stream or not
     */
    bool m_connected = false;

    /**
     * @brief Heartbits left till initialization happens
     *
     * Initial value is 10. We count down the heartbits to give the IMU some
     * time to prepare before we initialize the MAVLink data stream
     */
    quint32 m_heartbitCounter;

    /**
     * @brief Lost heartbits counter
     *
     * We count sequentially lost heartbits to see if the connection to the IMU
     * was really lost or if it is just a temporary one-time transmission
     * failure.
     */
    quint32 m_lostCounter;

    /**
     * @brief Primary MAVLink interface used to communicate with IMU
     */
    MavlinkInterface *m_mavlinkInterface = Q_NULLPTR;

    /**
     * @brief Heartbit timeout timer to indicate the loss of a heartbit
     */
    QTimer m_lifeTimer;

    /**
     * @brief Network access manager to interface with an HTTP server
     */
    QNetworkAccessManager m_net;
};

#endif // #ifndef CORE_H
