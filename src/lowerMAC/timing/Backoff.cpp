/******************************************************************************
 * Glue                                                                       *
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 16, D-52074 Aachen, Germany
 * phone: ++49-241-80-27910,
 * fax: ++49-241-80-22242
 * email: info@openwns.org
 * www: http://www.openwns.org
 * _____________________________________________________________________________
 *
 * openWNS is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 2 as published by the
 * Free Software Foundation;
 *
 * openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <WIFIMAC/lowerMAC/timing/Backoff.hpp>

#include <WNS/Assure.hpp>
#include <boost/bind.hpp>

using namespace wifimac::lowerMAC::timing;

Backoff::Backoff(BackoffObserver* _backoffObserver, const wns::pyconfig::View& _config) :
    backoffObserver(_backoffObserver),
    slotDuration(_config.get<wns::simulator::Time>("myConfig.slotDuration")),
    aifsDuration(_config.get<wns::simulator::Time>("myConfig.aifsDuration")),
    eifsDuration(_config.get<wns::simulator::Time>("myConfig.eifsDuration")),
    backoffFinished(false),
    transmissionWaiting(false),
    duringAIFS(false),
    rxError(false),
    cwMin(_config.get<int>("myConfig.cwMin")),
    cwMax(_config.get<int>("myConfig.cwMax")),
    cw(cwMin),
    uniform(0.0, 1.0, wns::simulator::getRNG()),
    logger(_config.get("backoffLogger")),
    // we start with an idle channel
    channelIsBusy(false),
    counter(0)
{
    assureNotNull(backoffObserver);
    MESSAGE_SINGLE(NORMAL, logger, "created");

    // start the initial backoff
    startNewBackoffCountdown(aifsDuration);
}


Backoff::~Backoff()
{
}

wns::simulator::Time Backoff::finishedAt() const
{
    wns::simulator::Time now = wns::simulator::getEventScheduler()->getTime();
    if (not channelIsBusy)
    {
        if (duringAIFS)
        {
            if(not rxError)
            {
                return now + cw*slotDuration + (aifsDuration - (now - aifsStart));
            }
            else
            {
                return now + cw*slotDuration + (eifsDuration - (now - aifsStart));
            }
        }
        else
        {
            if (backoffFinished == true)
            {
                return now;
            }
            else
            {
                return now + counter * slotDuration;
            }
        }
    }
    return wns::simulator::Time();
}


void
Backoff::startNewBackoffCountdown(wns::simulator::Time ifsDuration)
{
    assure(not channelIsBusy, "channel must be idle to start backoff");

    backoffFinished = false;
    duringAIFS = true;

    MESSAGE_SINGLE(NORMAL, logger, "Start new backoff, waiting for AIFS=" << ifsDuration);
    // First stage: Try to survive AIFS
    setNewTimeout(ifsDuration);
}

void
Backoff::onTimeout()
{
    assure(backoffFinished == false, "timer expired but not in backoff countdown");

    if(duringAIFS)
    {
        duringAIFS = false;
        // the constant waiting time has expired
        if (counter == 0)
        {
            if (not transmissionWaiting)
            {
                // obviously, no (re-)transmission is pending, so reset the cw
                cw = cwMin;
            }

            // we need a fresh counter, so we draw the random waiting
            // time between [0 and cw]
            counter = int(uniform() * (cw+1));
            if(counter > cw)
            {
                // corner case that uniform() gives exactly 1
                --counter;
            }
            assure(counter>=0, "counter too small");
            assure(counter<=cw, "counter too big");

            MESSAGE_SINGLE(NORMAL, logger, "AIFS waited, start new backoff with counter " << counter << ", cw is " << cw);
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, logger, "AIFS waited, continue backoff with counter " << counter);
        }
    }
    else
    {
        // one further slot waited
        --counter;
        MESSAGE_SINGLE(NORMAL, logger, "Slot waited, counter is now " << counter);
    }

    if(counter == 0)
    {
        // backoff finished
        backoffFinished = true;

        MESSAGE_SINGLE(NORMAL, logger,"Backoff has finished, transmission is " <<
                       ((transmissionWaiting) ? "waiting" : "none"));
        for(int i=0; i < eobObserver.size(); i++)
        {
            eobObserver[i]->backoffExpired();
        }

        if (transmissionWaiting)
        {
            transmissionWaiting = false;
            backoffFinished = false;
            backoffObserver->backoffExpired();
        }
    }
    else
    {
        setTimeout(slotDuration);
    }
}

bool
Backoff::transmissionRequest(const int transmissionCounter)
{
    transmissionWaiting = true;

    assure(transmissionCounter >= 1, "transmissionCounter must be >= 1");

    cw = cwMin;
    // retransmission, compute cw
    for (int i = 1; i < transmissionCounter; ++i)
    {
        cw = cw * 2 + 1;
        if(cw > cwMax)
        {
            cw = cwMax;
            break;
        }
    }


    MESSAGE_SINGLE(NORMAL, logger, "Data, transmission number " << transmissionCounter << " --> contention window = " << cw << " slots");

    if (backoffFinished and (not channelIsBusy))
    {
        // Postbackoff has expired, the medium is idle... go!
        MESSAGE_SINGLE(NORMAL, logger, "Post-Backoff has expired -> direct go for one frame");
        transmissionWaiting = false;
        backoffFinished = false;
        return true;
    }
    return false;
}

void
Backoff::onChannelIdle()
{
    assure(channelIsBusy, "Incoming onChannelIdle although channel is already idle");
    channelIsBusy = false;

    // start post-backoff
    aifsStart = wns::simulator::getEventScheduler()->getTime();
    if(rxError)
    {
        rxError = false;
        startNewBackoffCountdown(eifsDuration);
    }
    else
    {
        startNewBackoffCountdown(aifsDuration);
    }
}

void Backoff::onChannelBusy()
{
    channelIsBusy = true;

    wns::simulator::getEventScheduler()->scheduleDelay(
        boost::bind(&wifimac::lowerMAC::timing::Backoff::channelBusyDelay, this),
        1e-6);
}

void Backoff::channelBusyDelay()
{
    aifsStart = wns::simulator::Time();
    if(channelIsBusy and hasTimeoutSet())
    {
        // abort countdown
        cancelTimeout();
        MESSAGE_SINGLE(NORMAL, logger, "Channel busy detected during countdown -> freeze.");
    }
}

void
Backoff::onRxStart(wns::simulator::Time /*expRxTime*/)
{

}

void
Backoff::onRxEnd()
{

}

void
Backoff::onRxError()
{
    MESSAGE_SINGLE(NORMAL, logger, "onRxError -> start next bo with eifs=" << eifsDuration);
    rxError = true;
    if(not  backoffFinished and duringAIFS and aifsStart == wns::simulator::getEventScheduler()->getTime())
    {
        // got onChannelIdle signal before onRxError signal
        cancelTimeout();
        startNewBackoffCountdown(eifsDuration);
    }
}

void Backoff::registerEOBObserver(BackoffObserver * observer) 
{
    eobObserver.push_back(observer);
}
