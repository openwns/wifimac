/******************************************************************************
 * WiFiMac                                                                    *
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

#include <WIFIMAC/management/SINRInformationBase.hpp>

using namespace wifimac::management;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::management::SINRInformationBase,
    wns::ldk::ManagementServiceInterface,
    "wifimac.management.SINRInformationBase",
    wns::ldk::MSRConfigCreator);

SINRInformationBase::SINRInformationBase( wns::ldk::ManagementServiceRegistry* msr, const wns::pyconfig::View& config):
    wns::ldk::ManagementService(msr),
    logger(config.get("logger")),
    windowSize(config.get<simTimeType>("windowSize"))
{

}

void
SINRInformationBase::onMSRCreated()
{
    MESSAGE_SINGLE(NORMAL, logger, "Created.");
}

void
SINRInformationBase::putMeasurement(const wns::service::dll::UnicastAddress tx,
                                    const wns::Ratio sinr,
                                    const wns::simulator::Time estimatedValidity)
{
    assure(tx.isValid(), "Address is not valid");

    if(!measuredSINRHolder.knows(tx))
    {
        measuredSINRHolder.insert(tx, new wns::SlidingWindow(windowSize));
    }
    measuredSINRHolder.find(tx)->put(sinr.get_dB());

    if(estimatedValidity > 0.0)
    {
        if(not lastMeasurement.knows(tx))
        {
            lastMeasurement.insert(tx, new ratioTimePair);
        }
        lastMeasurement.find(tx)->first = sinr;
        lastMeasurement.find(tx)->second = wns::simulator::getEventScheduler()->getTime() + estimatedValidity;
    }
}

bool
SINRInformationBase::knowsMeasuredSINR(const wns::service::dll::UnicastAddress tx)
{
    assure(tx.isValid(), "Address is not valid");

    if(lastMeasurement.knows(tx) and
       (lastMeasurement.find(tx)->second > wns::simulator::getEventScheduler()->getTime()))
    {
        return true;
    }

    if(measuredSINRHolder.knows(tx))
    {
        return(measuredSINRHolder.find(tx)->getNumSamples() > 0);
    }
    else
    {
        return(false);
    }
}

wns::Ratio
SINRInformationBase::getMeasuredSINR(const wns::service::dll::UnicastAddress tx)
{
    assure(this->knowsMeasuredSINR(tx), "SINR for transmitter " << tx << " not known");

    if(lastMeasurement.knows(tx) and
       (lastMeasurement.find(tx)->second > wns::simulator::getEventScheduler()->getTime()))
    {
        return lastMeasurement.find(tx)->first;
    }
    return(wns::Ratio::from_dB(measuredSINRHolder.find(tx)->getAbsolute() / measuredSINRHolder.find(tx)->getNumSamples()));
}

void
SINRInformationBase::putPeerSINR(const wns::service::dll::UnicastAddress peer,
                                 const wns::Ratio sinr,
                                 const wns::simulator::Time estimatedValidity)
{
    assure(peer.isValid(), "Address is not valid");

    if(!peerSINRHolder.knows(peer))
    {
        peerSINRHolder.insert(peer, new wns::Ratio(sinr));
    }
    else
    {
        *(peerSINRHolder.find(peer)) = sinr;
    }

    if(estimatedValidity > 0.0)
    {
        if(not lastPeerMeasurement.knows(peer))
        {
            lastPeerMeasurement.insert(peer, new ratioTimePair);
        }
        lastPeerMeasurement.find(peer)->first = sinr;
        lastPeerMeasurement.find(peer)->second = wns::simulator::getEventScheduler()->getTime() + estimatedValidity;
    }
}

bool
SINRInformationBase::knowsPeerSINR(const wns::service::dll::UnicastAddress peer)
{
    assure(peer.isValid(), "Address is not valid");

    if(fakePeerMeasurement.knows(peer) and
       (fakePeerMeasurement.find(peer)->second == wns::simulator::getEventScheduler()->getTime()))
    {
        return true;
    }

    if(lastPeerMeasurement.knows(peer) and
       (lastPeerMeasurement.find(peer)->second > wns::simulator::getEventScheduler()->getTime()))
    {
        return true;
    }

    return(peerSINRHolder.knows(peer));
}

wns::Ratio
SINRInformationBase::getPeerSINR(const wns::service::dll::UnicastAddress peer)
{
    assure(this->knowsPeerSINR(peer), "peerSINR for " << peer << " is not known");

    if(fakePeerMeasurement.knows(peer) and
       (fakePeerMeasurement.find(peer)->second == wns::simulator::getEventScheduler()->getTime()))
    {
        return fakePeerMeasurement.find(peer)->first;
    }

    if(lastPeerMeasurement.knows(peer) and
       (lastPeerMeasurement.find(peer)->second > wns::simulator::getEventScheduler()->getTime()))
    {
        return lastPeerMeasurement.find(peer)->first;
    }

    return(*(peerSINRHolder.find(peer)));
}

void
SINRInformationBase::putFakePeerSINR(const wns::service::dll::UnicastAddress peer,
                                     const wns::Ratio sinr)
{
    if(not fakePeerMeasurement.knows(peer))
    {
        fakePeerMeasurement.insert(peer, new ratioTimePair);
    }
    fakePeerMeasurement.find(peer)->first = sinr;
    fakePeerMeasurement.find(peer)->second = wns::simulator::getEventScheduler()->getTime();


}

