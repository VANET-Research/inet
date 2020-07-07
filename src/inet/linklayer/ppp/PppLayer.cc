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

#include "inet/common/INETDefs.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/ppp/PppFrame_m.h"
#include "inet/linklayer/ppp/PppLayer.h"

namespace inet {

Define_Module(PppLayer);

PppLayer::~PppLayer()
{
}

void PppLayer::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        registerService(Protocol::ppp, gate("upperLayerIn"), gate("upperLayerOut"));
        registerProtocol(Protocol::ppp, gate("lowerLayerOut"), gate("lowerLayerIn"));
    }
}

void PppLayer::handleMessage(cMessage *msg)
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

void PppLayer::encapsulate(Packet *packet)
{
    auto pppHeader = makeShared<PppHeader>();
    auto protocolTag = packet->getTagForUpdate<PacketProtocolTag>();
    pppHeader->setProtocol(ProtocolGroup::pppprotocol.getProtocolNumber(protocolTag->getProtocol()));
    packet->insertAtFront(pppHeader);
    auto pppTrailer = makeShared<PppTrailer>();
    packet->insertAtBack(pppTrailer);
    protocolTag->setProtocol(&Protocol::ppp);
}

void PppLayer::decapsulate(Packet *packet)
{
    const auto& pppHeader = packet->popAtFront<PppHeader>();
    const auto& pppTrailer = packet->popAtBack<PppTrailer>(PPP_TRAILER_LENGTH);
    (void)pppTrailer;

    auto payloadProtocol = ProtocolGroup::pppprotocol.getProtocol(pppHeader->getProtocol());
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
}

} // namespace inet

