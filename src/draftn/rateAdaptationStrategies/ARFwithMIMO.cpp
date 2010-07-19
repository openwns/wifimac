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

#include <WIFIMAC/draftn/rateAdaptationStrategies/ARFwithMIMO.hpp>
#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>

#include <algorithm>

using namespace wifimac::lowerMAC::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(ARFwithMIMO, IRateAdaptationStrategy, "ARFwithMIMO", IRateAdaptationStrategyCreator);

ARFwithMIMO::ARFwithMIMO(
    const wns::pyconfig::View& _config,
    wifimac::management::PERInformationBase* _per,
    wifimac::management::SINRInformationBase* _sinr,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    IRateAdaptationStrategy(_config, _per, _sinr, _manager, _phyUser, _logger),
    per(_per),
    arfTimer(_config.get<wns::simulator::Time>("arfTimer")),
    exponentialBackoff(_config.get<bool>("exponentialBackoff")),
    initialSuccessThreshold(_config.get<int>("initialSuccessThreshold")),
    maxSuccessThreshold(_config.get<int>("maxSuccessThreshold")),
    successThreshold(_config.get<int>("initialSuccessThreshold")),
    probePacket(false),
    logger(_logger),
    curPhyModeId(0),
    knowsReceiver(false)
{
    friends.phyUser = _phyUser;
    friends.manager = _manager;

    // create an ordered list of the available phyModes
    wifimac::convergence::PhyMode pm;
    std::vector<wifimac::convergence::PhyMode> allPMs;
    allPMs.clear();
    for (int i = 1; i <= friends.manager->getNumAntennas(); i++)
    {
        pm = friends.manager->getPhyUser()->getPhyModeProvider()->getDefaultPhyMode();
        while(not friends.manager->getPhyUser()->getPhyModeProvider()->hasLowestMCS(pm))
        {
            friends.manager->getPhyUser()->getPhyModeProvider()->mcsDown(pm);
        }
        pm.setUniformMCS(pm.getSpatialStreams()[0], i);
        while(!friends.manager->getPhyUser()->getPhyModeProvider()->hasHighestMCS(pm))
        {
            allPMs.push_back(pm);
            friends.manager->getPhyUser()->getPhyModeProvider()->mcsUp(pm);
        }
        allPMs.push_back(pm);

    }
    /*std::vector<wifimac::convergence::PhyMode>::iterator itr;
    for (int i = 0; i < pmList.size(); ++i)
    {
        for (itr = sinrSortedPMs.begin(); itr != sinrSortedPMs.end(); ++itr)
        {
            if (pmList[i].getMinSINR()-friends.manager->getPhyUser()->getExpectedPostSINRFactor(pmList[i].getNumberOfSpatialStreams(),friends.manager->getNumAntennas())
                < (*itr).getMinSINR()-friends.manager->getPhyUser()->getExpectedPostSINRFactor((*itr).getNumberOfSpatialStreams(),friends.manager->getNumAntennas()))
            {
                sinrSortedPMs.insert(itr,pmList[i]);
                break;
            }
        }
        if (itr == sinrSortedPMs.end())
        {
            sinrSortedPMs.push_back(pmList[i]);
        }
        }*/

    unsigned int maxdbps = 0;
    pmList.clear();
    for (std::vector<wifimac::convergence::PhyMode>::iterator itr = allPMs.begin();
         itr != allPMs.end();
         itr++)
    {
        if (maxdbps < (*itr).getDataBitsPerSymbol())
        {
            maxdbps = (*itr).getDataBitsPerSymbol();
            pmList.push_back((*itr));
        }
    }

    assure(successThreshold > 0, "Success threshold must be > 0");
}

wifimac::convergence::PhyMode
ARFwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver,
                        size_t numTransmissions,
                        const wns::Ratio /*lqm*/) const
{
    return(this->getPhyMode(receiver, numTransmissions));
}

wifimac::convergence::PhyMode
ARFwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver,
                        size_t numTransmissions) const
{

    if(not knowsReceiver)
    {
        std::vector<wifimac::convergence::PhyMode>* myPMList = new std::vector<wifimac::convergence::PhyMode>;
        unsigned int numTx = friends.manager->getNumAntennas();
        unsigned int numRx = 1;
        if(wifimac::management::TheVCIBService::Instance().getVCIB()->knows(receiver, "numAntennas"))
        {
            numRx = wifimac::management::TheVCIBService::Instance().getVCIB()->get<int>(receiver, "numAntennas");
        }
        unsigned int maxNumSS = (numTx < numRx) ? numTx : numRx;

        std::vector<wifimac::convergence::PhyMode>::const_iterator itr = pmList.begin();
        while(itr != pmList.end())
        {
            if(itr->getNumberOfSpatialStreams() <= maxNumSS)
            {
                myPMList->push_back(*itr);
                ++itr;
            }
        }
        return (this->getPhyMode(receiver, numTransmissions, *myPMList));
    }
    else
    {
        return (this->getPhyMode(receiver, numTransmissions, pmList));
    }
}

wifimac::convergence::PhyMode
ARFwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver,
                        size_t numTransmissions,
                        const std::vector<wifimac::convergence::PhyMode>& myPMList) const
{
    if((probePacket and numTransmissions == 2) or (numTransmissions >= 3))
    {
        // last frame did not succeed
        if (curPhyModeId > 0)
        {
            return myPMList[curPhyModeId-1];
        }
        else
        {
            return myPMList[curPhyModeId];
        }
    }

    if(per->getSuccessfull(receiver) == successThreshold)
    {
        // last frame did not succeed
        if (curPhyModeId < (pmList.size()-1))
        {
            return myPMList[curPhyModeId+1];
        }
        else
        {
            return myPMList[curPhyModeId];
        }
    }

    return myPMList[curPhyModeId];

}

void
ARFwithMIMO::setCurrentPhyMode(const wns::service::dll::UnicastAddress receiver, 
                               wifimac::convergence::PhyMode pm)
{
    if(not knowsReceiver)
    {
        // clear pmList of impossible PhyModes with too many streams

        unsigned int numTx = friends.manager->getNumAntennas();
        unsigned int numRx = 1;
        if(wifimac::management::TheVCIBService::Instance().getVCIB()->knows(receiver, "numAntennas"))
        {
            numRx = wifimac::management::TheVCIBService::Instance().getVCIB()->get<int>(receiver, "numAntennas");
        }
        unsigned int maxNumSS = (numTx < numRx) ? numTx : numRx;

        std::vector<wifimac::convergence::PhyMode>::iterator itr = pmList.begin();
        while(itr != pmList.end())
        {
            if(itr->getNumberOfSpatialStreams() > maxNumSS)
            {
                pmList.erase(itr);
                itr = pmList.begin();
            }
            else
            {
                ++itr;
            }
        }

        MESSAGE_BEGIN(NORMAL, *logger, m, "Initializing RA to " << receiver << " with PhyModes: ");
        for(itr = pmList.begin(); itr != pmList.end(); ++itr)
        {
            m << *itr << " ";
        }
        MESSAGE_END();

        knowsReceiver = true;
    }
    myReceiver = receiver;

    int nextPhyModeId = 0;
    for(; nextPhyModeId < pmList.size(); nextPhyModeId++)
    {
        if(pmList[nextPhyModeId] == pm)
        {
            break;
        }
    }
    assure(nextPhyModeId < pmList.size(), "Trying to set invalid PhyMode!");

    if(curPhyModeId == nextPhyModeId)
    {
        probePacket = false;
        return;
    }
    else
    {
        per->reset(receiver);

        if(nextPhyModeId < curPhyModeId)
        {
            if(not this->hasTimeoutSet())
            {
                this->setTimeout(arfTimer);
            }
            if(probePacket)
            {
                // last one was a probe packet, but did not succeed
                probePacket = false;
                MESSAGE_SINGLE(NORMAL, *logger, "Last probe packet to " << receiver << " did not succeed, going down to MCS "<< pmList[nextPhyModeId]);
                if(exponentialBackoff and successThreshold < maxSuccessThreshold)
                {
                    successThreshold = successThreshold*2;
                    MESSAGE_SINGLE(NORMAL, *logger, "Set successThreshold to " << successThreshold);
                }
            }
            else
                {
                MESSAGE_SINGLE(NORMAL, *logger, "Failed transmissions to " << receiver << " , going down to MCS "<< pmList[nextPhyModeId]);
                if(exponentialBackoff)
                {
                    successThreshold = successThreshold / 2;
                    if(successThreshold < initialSuccessThreshold)
                    {
                        successThreshold = initialSuccessThreshold;
                    }
                    MESSAGE_SINGLE(NORMAL, *logger, "Set successThreshold to " << successThreshold);
                }
            }
        }
        else
        {
            probePacket = true;

            if(this->hasTimeoutSet())
            {
                this->cancelTimeout();
            }

            MESSAGE_SINGLE(NORMAL, *logger, per->getSuccessfull(receiver) << " successfull transmissions to " << receiver << ", sending probe packet with " << pmList[nextPhyModeId]);
        }
        curPhyModeId = nextPhyModeId;
    }
}

void
ARFwithMIMO::onTimeout()
{
    if (curPhyModeId == pmList.size()-1)
    {
        return;
    }
    ++curPhyModeId;
    per->reset(myReceiver);
    probePacket = true;
    MESSAGE_SINGLE(NORMAL, *logger, "Timeout, set mcs up to " << pmList[curPhyModeId]);
}
