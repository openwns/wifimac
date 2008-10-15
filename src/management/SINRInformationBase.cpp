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
                                    const wns::Ratio sinr)
{
    assure(tx.isValid(), "Address is not valid");

    if(!measuredSINRHolder.knows(tx))
    {
        measuredSINRHolder.insert(tx, new wns::SlidingWindow(windowSize));
    }
    measuredSINRHolder.find(tx)->put(sinr.get_dB());
}

bool
SINRInformationBase::knowsMeasuredSINR(const wns::service::dll::UnicastAddress tx)
{
    assure(tx.isValid(), "Address is not valid");

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
SINRInformationBase::getAverageMeasuredSINR(const wns::service::dll::UnicastAddress tx)
{
    assure(this->knowsMeasuredSINR(tx), "SINR for transmitter " << tx << " not known");
    return(wns::Ratio::from_dB(measuredSINRHolder.find(tx)->getAbsolute() / measuredSINRHolder.find(tx)->getNumSamples()));
}

void
SINRInformationBase::putPeerSINR(const wns::service::dll::UnicastAddress peer,
                                 const wns::Ratio sinr)
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
}

bool
SINRInformationBase::knowsPeerSINR(const wns::service::dll::UnicastAddress peer)
{
    assure(peer.isValid(), "Address is not valid");

    return(peerSINRHolder.knows(peer));
}

wns::Ratio
SINRInformationBase::getAveragePeerSINR(const wns::service::dll::UnicastAddress peer)
{
    assure(this->knowsPeerSINR(peer), "peerSINR for " << peer << " is not known");
    return(*(peerSINRHolder.find(peer)));
}


