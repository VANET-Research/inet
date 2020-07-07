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

#ifndef __INET_PppLayer_H
#define __INET_PppLayer_H

#include "inet/common/INETDefs.h"
#include "inet/common/packet/Packet.h"

namespace inet {

/**
 * PPP implementation.
 */
class INET_API PppLayer : public cSimpleModule
{
  public:
    virtual ~PppLayer();

  protected:
    virtual void encapsulate(Packet *msg);
    virtual void decapsulate(Packet *packet);

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;
};

} // namespace inet

#endif // ifndef __INET_PppLayer_H

