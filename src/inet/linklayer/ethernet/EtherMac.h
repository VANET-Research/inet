/*
 * Copyright (C) 2003 Andras Varga; CTIE, Monash University, Australia
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __INET_ETHERMAC_H
#define __INET_ETHERMAC_H

#include "inet/common/INETDefs.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/EtherMacBase.h"

namespace inet {

class EthernetJamSignal;
class EtherPauseFrame;

/**
 * Ethernet MAC module which supports both half-duplex (CSMA/CD) and full-duplex
 * operation. (See also EtherMacFullDuplex which has a considerably smaller
 * code with all the CSMA/CD complexity removed.)
 *
 * See NED file for more details.
 */
class INET_API EtherMac : public EtherMacBase
{
  public:
    EtherMac() {}
    virtual ~EtherMac();

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void initializeFlags() override;
    virtual void initializeStatistics() override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;

  protected:
    // states
    int numConcurrentRxSignals = 0;    // number of colliding frames -- we must receive this many jams (caches endRxTimeList.size())
    int backoffs = 0;    // value of backoff for exponential back-off algorithm
    long currentSendPkTreeID = -1;

    // other variables
    EthernetSignal *frameBeingReceived = nullptr;
    cMessage *endBackoffMsg = nullptr;
    cMessage *endJammingMsg = nullptr;

    // list of receptions during reconnect state; an additional special entry (with packetTreeId=-1)
    // stores the end time of the reconnect state
    struct PkIdRxTime
    {
        long packetTreeId;    // >=0: tree ID of packet being received; -1: this is a special entry that stores the end time of the reconnect state
        simtime_t endTime;    // end of reception
        PkIdRxTime(long id, simtime_t time) { packetTreeId = id; endTime = time; }
    };
    typedef std::list<PkIdRxTime> EndRxTimeList;
    EndRxTimeList endRxTimeList;    // list of incoming packets, ordered by endTime

    // statistics
    simtime_t totalCollisionTime;    // total duration of collisions on channel
    simtime_t totalSuccessfulRxTxTime;    // total duration of successful transmissions on channel
    simtime_t channelBusySince;    // needed for computing totalCollisionTime/totalSuccessfulRxTxTime
    unsigned long numCollisions = 0;    // collisions (NOT number of collided frames!) sensed
    unsigned long numBackoffs = 0;    // number of retransmissions
    int framesSentInBurst = 0;    // Number of frames send out in current frame burst
    B bytesSentInBurst = B(0);    // Number of bytes transmitted in current frame burst

    static simsignal_t collisionSignal;
    static simsignal_t backoffSlotsGeneratedSignal;

  protected:
    // event handlers
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void handleEndIFGPeriod();
    virtual void handleEndPausePeriod();
    virtual void handleEndTxPeriod();
    virtual void handleEndRxPeriod();
    virtual void handleEndBackoffPeriod();
    virtual void handleEndJammingPeriod();
    virtual void handleRetransmission();

    // helpers
    virtual void startFrameTransmission();//OK1
    virtual void handleUpperPacket(Packet *pk) override;//OK1
    virtual void processMsgFromNetwork(physicallayer::SignalBase *signal);//OK1
    virtual void processRxSignalStart(const EthernetSignal *signal);
    virtual void processRxSignalEnd(EthernetSignal *signal);
    virtual void processReceivedDataFrame(Packet *packet);//OK1
    virtual void processPauseCommand(int pauseUnits);
    virtual void scheduleEndIFGPeriod();//OK1
    virtual void scheduleEndPausePeriod(int pauseUnits);//OK1
    virtual void beginSendFrames();//OK1
    // helpers
    virtual void readChannelParameters(bool errorWhenAsymmetric) override;
    virtual void fillIFGIfInBurst();
    virtual void scheduleEndTxPeriod(B sentFrameByteLength);
    virtual void scheduleEndRxPeriod(EthernetSignal *);
    virtual void sendJamSignal();
    virtual void frameReceptionComplete(EthernetSignal *signal);
    virtual void processRxJamStart(const EthernetJamSignal *msg);
    virtual void processReceivedJam(EthernetJamSignal *jam);
    virtual void processReceivedControlFrame(Packet *packet);
    virtual void processConnectDisconnect() override;
    virtual void addReceptionInReconnectState(long id, simtime_t endRxTime);
    virtual void processDetectedCollision();

    B calculateMinFrameLength();
    B calculatePaddedFrameLength(Packet *frame);

    virtual void printState();
};

} // namespace inet

#endif // ifndef __INET_ETHERMAC_H

