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

#ifndef __INET_LOOPBACK_H
#define __INET_LOOPBACK_H

#include "inet/common/INETDefs.h"

namespace inet {

/**
 * LoopbackLayer implementation.
 */
class INET_API LoopbackLayer : public cSimpleModule
{
  public:
    LoopbackLayer() {}
    virtual ~LoopbackLayer();

  protected:
    virtual void encapsulate(Packet *msg);
    virtual void decapsulate(Packet *packet);

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
};

} // namespace inet

#endif // ifndef __INET_LOOPBACK_H

