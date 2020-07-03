//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "inet/protocol/transceiver/StreamThroughTransmitter.h"

namespace inet {

Define_Module(StreamThroughTransmitter);

void StreamThroughTransmitter::initialize(int stage)
{
    PacketTransmitterBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        dataratePar = &par("datarate");
        txEndTimer = new cMessage("TxEndTimer");
    }
}

StreamThroughTransmitter::~StreamThroughTransmitter()
{
    cancelAndDelete(txEndTimer);
}

void StreamThroughTransmitter::handleMessage(cMessage *message)
{
    if (message == txEndTimer)
        endTx();
    else
        throw cRuntimeError("Unknown message");
}

void StreamThroughTransmitter::pushPacket(Packet *packet, cGate *gate)
{
    Enter_Method("pushPacket");
    take(packet);
//    throw cRuntimeError("Does not support");
//    if (isTransmitting())
//        abortTx();
    startTx(packet);
}

void StreamThroughTransmitter::pushPacketStart(Packet *packet, cGate *gate, bps datarate)
{
    Enter_Method("pushPacketStart");
    take(packet);
    startTx(packet);
}

void StreamThroughTransmitter::pushPacketEnd(Packet *packet, cGate *gate, bps datarate)
{
    Enter_Method("pushPacketEnd");
    take(packet);
    delete txSignal;
    txSignal = encodePacket(packet);
    cancelEvent(txEndTimer);
    scheduleTxEndTimer(txSignal);
}

void StreamThroughTransmitter::pushPacketProgress(Packet *packet, cGate *gate, bps datarate, b position, b extraProcessableLength)
{
    Enter_Method("pushPacketProgress");
    take(packet);
    delete txSignal;
    txSignal = encodePacket(packet);
    clocktime_t timePosition = getClockTime() - txStartTime;
    b bitPosition = b(std::floor(datarate.get() * timePosition.dbl()));
    sendPacketProgress(txSignal, bitPosition, timePosition);
    cancelEvent(txEndTimer);
    scheduleTxEndTimer(txSignal);
}

void StreamThroughTransmitter::startTx(Packet *packet)
{
    datarate = bps(*dataratePar);
    txStartTime = getClockTime();
    txSignal = encodePacket(packet);
    EV_INFO << "Starting transmission: packetName = " << packet->getName() << ", length = " << packet->getTotalLength() << ", duration = " << txSignal->getDuration() << std::endl;
    scheduleTxEndTimer(txSignal);
    emit(transmissionStartedSignal, txSignal);
    sendPacketStart(txSignal->dup());
}

void StreamThroughTransmitter::endTx()
{
    auto packet = check_and_cast<Packet *>(txSignal->getEncapsulatedPacket());
    EV_INFO << "Ending transmission: packetName = " << packet->getName() << std::endl;
    emit(transmissionEndedSignal, txSignal);
    sendPacketEnd(txSignal);
    txSignal = nullptr;
    txId = -1;
    txStartTime = -1;
    // TODO: dup packet?
    producer->handlePushPacketProcessed(packet, inputGate->getPathStartGate(), true);
    producer->handleCanPushPacket(inputGate->getPathStartGate());
}

//void StreamThroughTransmitter::abortTx()
//{
//    cancelEvent(txEndTimer);
//    auto packet = check_and_cast<Packet *>(txSignal->getEncapsulatedPacket());
//    b transmittedLength = getPushPacketProcessedLength(packet, inputGate);
//    packet->eraseAtBack(packet->getTotalLength() - transmittedLength);
//    auto signal = encodePacket(packet);
//    EV_INFO << "Aborting transmission: packetName = " << packet->getName() << ", length = " << packet->getTotalLength() << ", duration = " << signal->getDuration() << std::endl;
//    emit(transmissionEndedSignal, signal);
//    sendPacketEnd(signal);
//    producer->handlePushPacketProcessed(packet, inputGate->getPathStartGate(), false);
//    txStartTime = -1;
//    producer->handleCanPushPacket(inputGate->getPathStartGate());
//}

clocktime_t StreamThroughTransmitter::calculateDuration(const Packet *packet) const
{
    return packet->getTotalLength().get() / datarate.get();
}

void StreamThroughTransmitter::scheduleTxEndTimer(Signal *signal)
{
    ASSERT(txStartTime != -1);
    scheduleClockEvent(txStartTime + SIMTIME_AS_CLOCKTIME(signal->getDuration()), txEndTimer);
}

b StreamThroughTransmitter::getPushPacketProcessedLength(Packet *packet, cGate *gate)
{
    if (txSignal == nullptr)
        return b(0);
    clocktime_t transmissionDuration = getClockTime() - txStartTime;
    return b(std::floor(datarate.get() * transmissionDuration.dbl()));
}

} // namespace inet

