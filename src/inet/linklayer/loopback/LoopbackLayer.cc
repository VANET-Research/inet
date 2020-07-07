//
// Copyright (C) 2020 OpenSim Ltd.
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
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//
// Author: Zoltan Bojthe
//

#include "inet/common/INETDefs.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/loopback/LoopbackLayer.h"

namespace inet {

Define_Module(LoopbackLayer);

LoopbackLayer::~LoopbackLayer()
{
}

void LoopbackLayer::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        registerService(Protocol::loopback, gate("upperLayerIn"), gate("upperLayerOut"));
        registerProtocol(Protocol::loopback, gate("lowerLayerOut"), gate("lowerLayerIn"));
    }
}

void LoopbackLayer::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("upperLayerIn")) {
        Packet *packet = check_and_cast<Packet *>(msg);
        packet->removeTag<DispatchProtocolReq>();
        encapsulate(packet);
        EV_INFO << "Sending " << packet << " to lower layer.\n";
        send(packet, "lowerLayerOut");
    }
    else if (msg->arrivedOn("lowerLayerIn")) {
        Packet *packet = check_and_cast<Packet *>(msg);
        decapsulate(packet);
        EV_INFO << "Sending " << packet << " to upper layer.\n";
        send(packet, "upperLayerOut");
    }
}

void LoopbackLayer::encapsulate(Packet *packet)
{
}

void LoopbackLayer::decapsulate(Packet *packet)
{
    auto payloadProtocol = packet->getTag<PacketProtocolTag>()->getProtocol();
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
}

} // namespace inet

