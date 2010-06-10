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

#include <WIFIMAC/draftn/SINRwithMIMOInformationBase.hpp>

using namespace wifimac::draftn;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::SINRwithMIMOInformationBase,
    wns::ldk::ManagementServiceInterface,
    "wifimac.draftn.SINRwithMIMOInformationBase",
    wns::ldk::MSRConfigCreator);

SINRwithMIMOInformationBase::SINRwithMIMOInformationBase( wns::ldk::ManagementServiceRegistry* msr,
                                                          const wns::pyconfig::View& config ):
    wifimac::management::SINRInformationBase(msr, config),
    logger(config.get("logger"))
{

}

void
SINRwithMIMOInformationBase::onMSRCreated()
{

}

void
SINRwithMIMOInformationBase::putFactorMeasurement(const wns::service::dll::UnicastAddress tx,
                                                  const std::vector<wns::Ratio> sinrFactor)
{
    assure(tx.isValid(), "Address is not valid");
    if(not factorsMeasurement.knows(tx))
    {
        factorsMeasurement.insert(tx, new NumSSToFactorMap());
    }
    (*factorsMeasurement.find(tx))[sinrFactor.size()] = sinrFactor;
}

bool
SINRwithMIMOInformationBase::knowsMeasuredFactor(const wns::service::dll::UnicastAddress tx,
                                                 unsigned int numSS) const
{
    assure(tx.isValid(), "Address is not valid");
    if(factorsMeasurement.knows(tx))
    {
        return (factorsMeasurement.find(tx)->count(numSS) == 1);
    }
    return(false);
}



std::vector<wns::Ratio>
SINRwithMIMOInformationBase::getMeasuredFactor(const wns::service::dll::UnicastAddress tx,
                                               unsigned int numSS) const
{
    assure(this->knowsMeasuredFactor(tx, numSS), "Factor for transmitter " << tx << " not known");
    return ((*factorsMeasurement.find(tx))[numSS]);
}

void
SINRwithMIMOInformationBase::putPeerFactor(const wns::service::dll::UnicastAddress peer,
                                           const std::vector<wns::Ratio> factor)
{
    if(not peerFactors.knows(peer))
    {
        peerFactors.insert(peer, new NumSSToFactorMap());
    }
    (*peerFactors.find(peer))[factor.size()] = factor;
}

bool
SINRwithMIMOInformationBase::knowsPeerFactor(const wns::service::dll::UnicastAddress peer,
                                             unsigned int numSS) const
{
    if(fakePeerFactors.knows(peer) and
       fakePeerFactors.find(peer)->second == wns::simulator::getEventScheduler()->getTime())
    {
        return(fakePeerFactors.find(peer)->first.count(numSS) == 1);
    }

    if(peerFactors.knows(peer))
    {
        return (peerFactors.find(peer)->count(numSS) == 1);
    }

    return(false);
}


std::vector<wns::Ratio>
SINRwithMIMOInformationBase::getPeerFactor(const wns::service::dll::UnicastAddress peer,
                                           unsigned int numSS) const
{
    assure(this->knowsPeerFactor(peer, numSS), "Factor for peer " << peer << " not known");

    if(fakePeerFactors.knows(peer) and
       fakePeerFactors.find(peer)->second == wns::simulator::getEventScheduler()->getTime() and
       fakePeerFactors.find(peer)->first.count(numSS) == 1)
    {
        return (fakePeerFactors.find(peer)->first)[numSS];
    }

    return ((*peerFactors.find(peer))[numSS]);
}

void
SINRwithMIMOInformationBase::putFakePeerMIMOFactors(const wns::service::dll::UnicastAddress peer,
                                                    NumSSToFactorMap allFactors)
{
    if(not fakePeerFactors.knows(peer))
    {
        fakePeerFactors.insert(peer, new FactorsTimePair);
    }
    fakePeerFactors.find(peer)->first = allFactors;
    fakePeerFactors.find(peer)->second = wns::simulator::getEventScheduler()->getTime();
}

std::map<unsigned int, std::vector<wns::Ratio> >
SINRwithMIMOInformationBase::getAllMeasuredFactors(const wns::service::dll::UnicastAddress peer) const
{
    assure(factorsMeasurement.knows(peer), "Factor for transmitter " << peer << " not known");
    return (*factorsMeasurement.find(peer));
}
