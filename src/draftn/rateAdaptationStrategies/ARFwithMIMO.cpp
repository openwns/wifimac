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

using namespace wifimac::draftn::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(ARFwithMIMO, 
                                     wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategy,
                                     "ARFwithMIMO",
                                     wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategyCreator);

ARFwithMIMO::ARFwithMIMO(
    const wns::pyconfig::View& _config,
    wns::service::dll::UnicastAddress _receiver,
    wifimac::management::PERInformationBase* _per,
    wifimac::management::SINRInformationBase* _sinr,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategy(_config, _receiver, _per, _sinr, _manager, _phyUser, _logger),
    per(_per),
    arfTimer(_config.get<wns::simulator::Time>("arfTimer")),
    exponentialBackoff(_config.get<bool>("exponentialBackoff")),
    initialSuccessThreshold(_config.get<int>("initialSuccessThreshold")),
    maxSuccessThreshold(_config.get<int>("maxSuccessThreshold")),
    successThreshold(_config.get<int>("initialSuccessThreshold")),
    myReceiver(_receiver),
    probePacket(false),
    logger(_logger),
    curPhyModeId(0),
    timeout(false)
{
    friends.phyUser = _phyUser;
    friends.manager = _manager;

    unsigned int numTx = friends.manager->getNumAntennas();
    unsigned int numRx = 1;
    if(wifimac::management::TheVCIBService::Instance().getVCIB()->knows(myReceiver, "numAntennas"))
    {
        numRx = wifimac::management::TheVCIBService::Instance().getVCIB()->get<int>(myReceiver, "numAntennas");
    }
    unsigned int maxNumSS = (numTx < numRx) ? numTx : numRx;

    // create an ordered list of the available phyModes
    wifimac::convergence::PhyMode pm;
    std::vector<wifimac::convergence::PhyMode> allPMs;
    allPMs.clear();
    for (int i = 1; i <= maxNumSS; i++)
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

    // keep only PhyModes with non-overlapping dbps
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
ARFwithMIMO::getPhyMode(size_t numTransmissions,
                        const wns::Ratio /*lqm*/) const
{
    return(this->getPhyMode(numTransmissions));
}

wifimac::convergence::PhyMode
ARFwithMIMO::getPhyMode(size_t numTransmissions) const
{
    if(timeout)
    {
        if (curPhyModeId < (pmList.size()-1))
        {
            return pmList[curPhyModeId+1];
        }
        else
        {
            return pmList[curPhyModeId];
        }
    }

    if((probePacket and numTransmissions == 2) or (numTransmissions >= 3))
    {
        // last frame did not succeed
        if (curPhyModeId > 0)
        {
            return pmList[curPhyModeId-1];
        }
        else
        {
            return pmList[curPhyModeId];
        }
    }

    if(per->getSuccessfull(myReceiver) == successThreshold)
    {
        // last frame did not succeed
        if (curPhyModeId < (pmList.size()-1))
        {
            return pmList[curPhyModeId+1];
        }
        else
        {
            return pmList[curPhyModeId];
        }
    }

    return pmList[curPhyModeId];

}

void
ARFwithMIMO::setCurrentPhyMode(wifimac::convergence::PhyMode pm)
{
    timeout = false;

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
        if(this->hasTimeoutSet())
        {
            this->cancelTimeout();
        }

        if(nextPhyModeId < curPhyModeId)
        {
            // set new timeout for switching up
            this->setTimeout(arfTimer);

            if(probePacket)
            {
                // last one was a probe packet, but did not succeed
                probePacket = false;
                MESSAGE_SINGLE(NORMAL, *logger, "Last probe packet to " << myReceiver << " did not succeed, going down to MCS "<< pmList[nextPhyModeId]);
                if(exponentialBackoff and successThreshold < maxSuccessThreshold)
                {
                    successThreshold = successThreshold*2;
                    MESSAGE_SINGLE(NORMAL, *logger, "Set successThreshold to " << successThreshold);
                }
            }
            else
            {
                MESSAGE_SINGLE(NORMAL, *logger, "Failed transmissions to " << myReceiver << " , going down to MCS "<< pmList[nextPhyModeId]);
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
            MESSAGE_SINGLE(NORMAL, *logger, per->getSuccessfull(myReceiver) << " successfull transmissions to " << myReceiver << ", sending probe packet with " << pmList[nextPhyModeId]);
        }
        curPhyModeId = nextPhyModeId;
        per->reset(myReceiver);

    }
}

void
ARFwithMIMO::onTimeout()
{
    if (curPhyModeId == pmList.size()-1)
    {
        return;
    }
    timeout = true;
    MESSAGE_SINGLE(NORMAL, *logger, "Timeout");
}
