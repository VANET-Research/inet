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

#include "inet/common/clock/SettableLinearClock.h"

namespace inet {

Define_Module(SettableLinearClock);

void SettableLinearClock::initialize()
{
    origin = par("origin");
    driftRate = par("driftRate").doubleValue() / 1e6;
}

clocktime_t SettableLinearClock::getClockTime() const
{
    simtime_t t = simTime();
    return originClock + ClockTime::from((t-origin) / (1 + driftRate));
}

void SettableLinearClock::scheduleClockEvent(clocktime_t t, cMessage *msg)
{
//TODO remove old items from arrivalTime
    simtime_t st = origin + (t - originClock).asSimTime() * (1 + driftRate);
    getTargetModule()->scheduleAt(st, msg);
    arrivalTimes[msg].clocktime = t;
}

cMessage *SettableLinearClock::cancelClockEvent(cMessage *msg)
{
    arrivalTimes.erase(msg);
    return getTargetModule()->cancelEvent(msg);
}

clocktime_t SettableLinearClock::getArrivalClockTime(cMessage *msg) const
{
    ASSERT(msg->isScheduled());
    return arrivalTimes.at(msg).clocktime;
}

void SettableLinearClock::purgeTimers()
{
    simtime_t now = simTime();
    for (auto it = arrivalTimes.begin(); it != arrivalTimes.end(); ) {
        if (it->second.simtime <= now)
            it = arrivalTimes.erase(it);
    }
}

void SettableLinearClock::rescheduleTimers()
{
    simtime_t now = simTime();
    for (auto it = arrivalTimes.begin(); it != arrivalTimes.end(); ) {
        if (it->second.simtime <= now)
            it = arrivalTimes.erase(it);
        else {
            cMessage * msg = it->first;
            simtime_t st = origin + (it->second.clocktime - originClock).asSimTime() * (1 + driftRate);
            if (st < now)
                st = now;
            getTargetModule()->cancelEvent(msg);
            getTargetModule()->scheduleAt(st, msg);
            it->second.simtime = st;
            ++it;
        }
    }
}

void SettableLinearClock::setDriftRate(double newDriftRate)
{
    driftRate = newDriftRate;
    rescheduleTimers();
}

void SettableLinearClock::setClockTime(clocktime_t t)
{
    origin = simTime();
    originClock = t;
    rescheduleTimers();
}

} // namespace inet

