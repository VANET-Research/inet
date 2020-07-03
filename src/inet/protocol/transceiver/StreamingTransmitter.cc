//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
//

#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/protocol/transceiver/StreamingTransmitter.h"

namespace inet {

Define_Module(StreamingTransmitter);

void StreamingTransmitter::initialize(int stage)
{
    PacketTransmitterBase::initialize(stage);
    OperationalMixin::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        dataratePar = &par("datarate");
        datarate = bps(*dataratePar);
        txEndTimer = new cMessage("TxEndTimer");
    }
}

StreamingTransmitter::~StreamingTransmitter()
{
    cancelAndDelete(txEndTimer);
    delete txSignal;
}

void StreamingTransmitter::handleMessageWhenUp(cMessage *message)
{
    if (message == txEndTimer) {
        endTx();
        producer->handleCanPushPacket(inputGate->getPathStartGate());
    }
    else
        PacketTransmitterBase::handleMessage(message);
}

void StreamingTransmitter::pushPacket(Packet *packet, cGate *gate)
{
    Enter_Method("pushPacket");
    take(packet);
    ASSERT(txSignal == nullptr);

    datarate = bps(*dataratePar);    // refresh stored datarate before start packet sending
    startTx(packet);
}

void StreamingTransmitter::startTx(Packet *packet)
{
    datarate = bps(*dataratePar);    //TODO for refresh datarate before start packet sending

    ASSERT(txSignal == nullptr);
    txStartTime = simTime();
    txSignal = encodePacket(packet);
    EV_INFO << "Starting transmission: packetName = " << packet->getName() << ", length = " << packet->getTotalLength() << ", duration = " << txSignal->getDuration() << std::endl;
    scheduleTxEndTimer(txSignal);
    emit(transmissionStartedSignal, txSignal);
    sendPacketStart(txSignal->dup());
}

void StreamingTransmitter::endTx()
{
    EV_INFO << "Ending transmission: packetName = " << txSignal->getName() << std::endl;
    emit(transmissionEndedSignal, txSignal);
    auto packet = check_and_cast<Packet *>(txSignal->getEncapsulatedPacket());
    producer->handlePushPacketProcessed(packet, inputGate->getPathStartGate(), true);
    sendPacketEnd(txSignal);
    txSignal = nullptr;
    txStartTime = -1;
}

void StreamingTransmitter::abortTx()
{
    ASSERT(txSignal != nullptr);
    cancelEvent(txEndTimer);
    auto packet = check_and_cast<Packet *>(txSignal->getEncapsulatedPacket());
    b transmittedLength = getPushPacketProcessedLength(packet, inputGate);
    packet->eraseAtBack(packet->getTotalLength() - transmittedLength);
    auto signal = encodePacket(packet);
    EV_INFO << "Aborting transmission: packetName = " << packet->getName() << ", length = " << packet->getTotalLength() << ", duration = " << signal->getDuration() << std::endl;
    emit(transmissionEndedSignal, signal);
    producer->handlePushPacketProcessed(packet, inputGate->getPathStartGate(), true);
    sendPacketEnd(txSignal);
    txSignal = nullptr;
    txStartTime = -1;
}

clocktime_t StreamingTransmitter::calculateDuration(const Packet *packet) const
{
    return packet->getDataLength().get() / datarate.get();
}

void StreamingTransmitter::scheduleTxEndTimer(Signal *signal)
{
    if (txEndTimer->isScheduled())
        cancelEvent(txEndTimer);
    scheduleClockEvent(getClockTime() + SIMTIME_AS_CLOCKTIME(signal->getDuration()), txEndTimer);
}

void StreamingTransmitter::pushPacketProgress(Packet *packet, cGate *gate, bps datarate, b position, b extraProcessableLength)
{
    take(packet);
    delete packet;
    // TODO:
}

b StreamingTransmitter::getPushPacketProcessedLength(Packet *packet, cGate *gate)
{
    if (txSignal == nullptr)
        return b(0);
    simtime_t transmissionDuration = simTime() - txStartTime;
    return b(std::floor(datarate.get() * transmissionDuration.dbl()));
}

void StreamingTransmitter::handleStartOperation(LifecycleOperation *operation)
{
    producer->handleCanPushPacket(inputGate->getPathStartGate());
}

void StreamingTransmitter::handleStopOperation(LifecycleOperation *operation)
{
    if (txSignal)
        abortTx();    //TODO: for finishing current transmission correctly, should wait the txEndTimer...
}

void StreamingTransmitter::handleCrashOperation(LifecycleOperation *operation)
{
    if (txSignal)
        abortTx();
}

} // namespace inet

