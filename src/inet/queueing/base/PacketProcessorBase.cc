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

#include "inet/common/ModuleAccess.h"
#include "inet/protocol/common/cProgress.h"
#include "inet/queueing/base/PacketProcessorBase.h"

namespace inet {
namespace queueing {

void PacketProcessorBase::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        displayStringTextFormat = par("displayStringTextFormat");
        numProcessedPackets = 0;
        processedTotalLength = b(0);
        WATCH(numProcessedPackets);
        WATCH(processedTotalLength);
    }
    else if (stage == INITSTAGE_LAST)
        updateDisplayString();
}

void PacketProcessorBase::handlePacketProcessed(Packet *packet)
{
    numProcessedPackets++;
    processedTotalLength += packet->getDataLength();
}

void PacketProcessorBase::checkPacketOperationSupport(cGate *gate) const
{
    auto startGate = findConnectedGate<IPacketProcessor>(gate, -1);
    auto endGate = findConnectedGate<IPacketProcessor>(gate, 1);
    auto startElement = startGate == nullptr ? nullptr : check_and_cast<IPacketProcessor *>(startGate->getOwnerModule());
    auto endElement = endGate == nullptr ? nullptr : check_and_cast<IPacketProcessor *>(endGate->getOwnerModule());
    if (startElement != nullptr && endElement != nullptr) {
        bool startPushing = startElement->supportsPacketPushing(startGate);
        bool startPulling = startElement->supportsPacketPulling(startGate);
        bool startPassing = startElement->supportsPacketPassing(startGate);
        bool startStreaming = startElement->supportsPacketStreaming(startGate);
        bool endPushing = endElement->supportsPacketPushing(endGate);
        bool endPulling = endElement->supportsPacketPulling(endGate);
        bool endPassing = endElement->supportsPacketPassing(endGate);
        bool endStreaming = endElement->supportsPacketStreaming(endGate);
        bool bothPushing = startPushing && endPushing;
        bool bothPulling = startPulling && endPulling;
        bool bothPassing = startPassing && endPassing;
        bool bothStreaming = startStreaming && endStreaming;
        bool eitherPushing = startPushing || endPushing;
        bool eitherPulling = startPulling || endPulling;
        bool eitherPassing = startPassing || endPassing;
        bool eitherStreaming = startStreaming || endStreaming;
        if (!bothPushing && !bothPulling) {
            if (eitherPushing) {
                if (startPushing)
                    throw cRuntimeError(endGate->getOwnerModule(), "doesn't support packet pushing on gate %s", endGate->getFullPath().c_str());
                if (endPushing)
                    throw cRuntimeError(startGate->getOwnerModule(), "doesn't support packet pushing on gate %s", startGate->getFullPath().c_str());
            }
            else if (eitherPulling) {
                if (startPulling)
                    throw cRuntimeError(endGate->getOwnerModule(), "doesn't support packet pulling on gate %s", endGate->getFullPath().c_str());
                if (endPulling)
                    throw cRuntimeError(startGate->getOwnerModule(), "doesn't support packet pulling on gate %s", startGate->getFullPath().c_str());
            }
            else
                throw cRuntimeError(endGate->getOwnerModule(), "neither supports packet pushing nor packet pulling on gate %s", gate->getFullPath().c_str());
        }
        if (!bothPassing && !bothStreaming) {
            if (eitherPassing) {
                if (startPassing)
                    throw cRuntimeError(endGate->getOwnerModule(), "doesn't support packet passing on gate %s", endGate->getFullPath().c_str());
                if (endPassing)
                    throw cRuntimeError(startGate->getOwnerModule(), "doesn't support packet passing on gate %s", startGate->getFullPath().c_str());
            }
            else if (eitherStreaming) {
                if (startStreaming)
                    throw cRuntimeError(endGate->getOwnerModule(), "doesn't support packet streaming on gate %s", endGate->getFullPath().c_str());
                if (endStreaming)
                    throw cRuntimeError(startGate->getOwnerModule(), "doesn't support packet streaming on gate %s", startGate->getFullPath().c_str());
            }
            else
                throw cRuntimeError(endGate->getOwnerModule(), "neither supports packet passing nor packet streaming on gate %s", gate->getFullPath().c_str());
        }
    }
    else if (startElement != nullptr && endElement == nullptr) {
        if (!startElement->supportsPacketSending(startGate))
            throw cRuntimeError(startGate->getOwnerModule(), "doesn't support packet sending on gate %s", startGate->getFullPath().c_str());
    }
    else if (startElement == nullptr && endElement != nullptr) {
        if (!endElement->supportsPacketSending(endGate))
            throw cRuntimeError(endGate->getOwnerModule(), "doesn't support packet sending on gate %s", endGate->getFullPath().c_str());
    }
    else
        throw cRuntimeError("Cannot check packet operation support on gate %s", gate->getFullPath().c_str());
}

void PacketProcessorBase::pushOrSendPacket(Packet *packet, cGate *gate, IPassivePacketSink *consumer)
{
    if (consumer != nullptr) {
        animateSendPacket(packet, gate);
        consumer->pushPacket(packet, gate->getPathEndGate());
    }
    else
        send(packet, gate);
}

void PacketProcessorBase::pushOrSendPacketStart(Packet *packet, cGate *gate, IPassivePacketSink *consumer, bps datarate)
{
    if (consumer != nullptr) {
        animateSendPacketStart(packet, gate, datarate);
        consumer->pushPacketStart(packet, gate->getPathEndGate(), datarate);
    }
    else
        // TODO: datarate
        send(packet, SendOptions().duration(packet->getDuration()), gate);
}

void PacketProcessorBase::pushOrSendPacketEnd(Packet *packet, cGate *gate, IPassivePacketSink *consumer, bps datarate)
{
    if (consumer != nullptr) {
        animateSendPacketEnd(packet, gate, datarate);
        consumer->pushPacketEnd(packet, gate->getPathEndGate(), datarate);
    }
    else
        send(packet, SendOptions().duration(packet->getDuration()), gate); // TODO: bps(datarate).get());
}

void PacketProcessorBase::pushOrSendPacketProgress(Packet *packet, cGate *gate, IPassivePacketSink *consumer, bps datarate, b position, b extraProcessableLength)
{
    if (consumer != nullptr) {
        animateSendPacketProgress(packet, gate, datarate, position, extraProcessableLength);
        consumer->pushPacketProgress(packet, gate->getPathEndGate(), datarate, position, extraProcessableLength);
    }
    else
        throw cRuntimeError("TODO");
        // TODO:
//        sendPacketProgress(packet, gate, 0, packet->getDuration(), bps(datarate).get(), b(position).get(), 0, b(extraProcessableLength).get());
}

void PacketProcessorBase::pushOrSendProgress(Packet *packet, cGate *gate, IPassivePacketSink *consumer, int progressKind, bps datarate, b position, b extraProcessableLength)
{
    if (consumer != nullptr) {
        animateSendProgress(packet, gate, progressKind, datarate, position, extraProcessableLength);
        pushProgress(packet, gate, consumer, progressKind, datarate, position, extraProcessableLength);
    }
    else
        throw cRuntimeError("TODO");
//        sendProgress(packet, gate, 0, packet->getDuration(), progressKind, bps(datarate).get(), b(position).get(), 0, b(extraProcessableLength).get(), 0);
}

void PacketProcessorBase::pushProgress(Packet *packet, cGate *gate, IPassivePacketSink *consumer, int progressKind, bps datarate, b position, b extraProcessableLength)
{
    switch (progressKind) {
        case cProgress::PACKET_START:
            consumer->pushPacketStart(packet, gate->getPathEndGate(), datarate);
            break;
        case cProgress::PACKET_END:
            consumer->pushPacketEnd(packet, gate->getPathEndGate(), datarate);
            break;
        case cProgress::PACKET_PROGRESS:
            consumer->pushPacketProgress(packet, gate->getPathEndGate(), datarate, position, extraProcessableLength);
            break;
        default: throw cRuntimeError("Unknown progress kind");
    }
}

void PacketProcessorBase::dropPacket(Packet *packet, PacketDropReason reason, int limit)
{
    PacketDropDetails details;
    details.setReason(reason);
    details.setLimit(limit);
    emit(packetDroppedSignal, packet, &details);
    delete packet;
}

void PacketProcessorBase::updateDisplayString() const
{
    if (getEnvir()->isGUI()) {
        auto text = StringFormat::formatString(displayStringTextFormat, this);
        getDisplayString().setTagArg("t", 0, text);
    }
}

void PacketProcessorBase::animateSend(cMessage *message, cGate *gate, simtime_t duration) const
{
    auto endGate = gate->getPathEndGate();
    message->setArrival(endGate->getOwnerModule()->getId(), endGate->getId(), simTime());
    auto envir = getEnvir();
    if (envir->isGUI()) {
        message->setSentFrom(gate->getOwnerModule(), gate->getId(), simTime());
        if (gate->getNextGate() != nullptr) {
            envir->beginSend(message);
            while (gate->getNextGate() != nullptr) {
                envir->messageSendHop(message, gate, 0, duration, false);
                gate = gate->getNextGate();
            }
            envir->endSend(message);
        }
    }
}

void PacketProcessorBase::animateSendPacket(Packet *packet, cGate *gate) const
{
    if (getEnvir()->isGUI())
        animateSend(packet, gate, 0);
}

void PacketProcessorBase::animateSendPacketStart(Packet *packet, cGate *gate, bps datarate) const
{
    animateSendProgress(packet, gate, cProgress::PACKET_START, datarate, b(0), b(0));
}

void PacketProcessorBase::animateSendPacketEnd(Packet *packet, cGate *gate, bps datarate) const
{
    animateSendProgress(packet, gate, cProgress::PACKET_END, datarate, packet->getDataLength(), b(0));
}

void PacketProcessorBase::animateSendPacketProgress(Packet *packet, cGate *gate, bps datarate, b position, b extraProcessableLength) const
{
    animateSendProgress(packet, gate, cProgress::PACKET_PROGRESS, datarate, position, extraProcessableLength);
}

void PacketProcessorBase::animateSendProgress(Packet *packet, cGate *gate, int progressKind, bps datarate, b position, b extraProcessableLength) const
{
    if (getEnvir()->isGUI()) {
        datarate = Mbps(100);
        simtime_t duration = s((packet->getTotalLength() - position) / datarate).get();
        packet->setDuration(duration);
        animateSend(packet, gate, duration);
//        auto progressMessage = createProgressMessage(packet, progressKind, datarate, position, extraProcessableLength);
//        animateSend(progressMessage, gate);
//        delete progressMessage;
    }
}

//cMessage *PacketProcessorBase::createProgressMessage(Packet *packet, int progressKind, bps datarate, b position, b extraProcessableLength) const
//{
//    std::string name = packet->getName();
//    switch (progressKind) {
//        case cProgress::PACKET_START: name += "-start"; break;
//        case cProgress::PACKET_END: name += "-end"; break;
//        case cProgress::PACKET_PROGRESS: name += "-progress"; break;
//    }
//    cProgress *progressMessage = new cProgress(name.c_str(), progressKind);
//    progressMessage->setPacket(packet->dup());
//    progressMessage->setDatarate(bps(datarate).get());
//    progressMessage->setBitPosition(b(position).get());
//    progressMessage->setTimePosition(-1);
//    progressMessage->setExtraProcessableBitLength(b(extraProcessableLength).get());
//    progressMessage->setExtraProcessableDuration(-1);
//    return progressMessage;
//}

const char *PacketProcessorBase::resolveDirective(char directive) const
{
    static std::string result;
    switch (directive) {
        case 'p':
            result = std::to_string(numProcessedPackets);
            break;
        case 'l':
            result = processedTotalLength.str();
            break;
        default:
            throw cRuntimeError("Unknown directive: %c", directive);
    }
    return result.c_str();
}

} // namespace queueing
} // namespace inet

