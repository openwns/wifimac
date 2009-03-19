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

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/OpportunisticwithMIMO.hpp>
#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>

#include <algorithm>

using namespace wifimac::lowerMAC::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(OpportunisticwithMIMO, IRateAdaptationStrategy, "OpportunisticwithMIMO", IRateAdaptationStrategyCreator);

OpportunisticwithMIMO::OpportunisticwithMIMO(
    wifimac::management::PERInformationBase* _per,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    IRateAdaptationStrategy(_per, _manager, _phyUser, _logger),
    per(_per),
    logger(_logger)
{
    friends.phyUser = _phyUser;
    friends.manager = _manager;
    curPhyModeId = friends.phyUser->getPhyModeProvider()->getLowestId();
    curSpatialStreams = 1;
}

wifimac::convergence::PhyMode
OpportunisticwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver, size_t numTransmissions, const wns::Ratio /*lqm*/)
{
    return(this->getPhyMode(receiver, numTransmissions));
}

wifimac::convergence::PhyMode
OpportunisticwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver, size_t numTransmissions)
{
    unsigned int numTx = friends.manager->getNumAntennas();
    unsigned int numRx = 1;
    if(wifimac::management::TheVCIBService::Instance().getVCIB()->knows(receiver, "numAntennas"))
    {
        numRx = wifimac::management::TheVCIBService::Instance().getVCIB()->get<int>(receiver, "numAntennas");
    }
    unsigned int maxNumSS = (numTx < numRx) ? numTx : numRx;

    assure(curPhyModeId >= friends.phyUser->getPhyModeProvider()->getLowestId(),
           "Invalid curPhyModeId");
    assure(curSpatialStreams >= 1, "Invalid curSpatialStreams");
    wifimac::convergence::PhyMode curPhyMode = friends.phyUser->getPhyModeProvider()->getPhyMode(curPhyModeId);
    curPhyMode.setNumberOfSpatialStreams(curSpatialStreams);

    if(not per->knowsPER(receiver))
    {
        assure(numTransmissions >= 1, "Must have at least one transmission");

        // if no information about the PER is available (due to a recent change),
        // we take the current phyMode and reduce it by the number of
        // retransmissions
        int phyModeId = curPhyModeId - (static_cast<int>(numTransmissions)-1);
        int ss = curSpatialStreams;
        if(phyModeId < friends.phyUser->getPhyModeProvider()->getLowestId())
        {
            if(ss > 1)
            {
                --ss;
                phyModeId = curPhyModeId;
            }
            else
            {
                phyModeId = friends.phyUser->getPhyModeProvider()->getLowestId();
            }
        }

        assure(phyModeId >= friends.phyUser->getPhyModeProvider()->getLowestId(),
               "Invalid phyModeId");
        assure(ss >= 1, "Invalid ss");

        wifimac::convergence::PhyMode phyMode = friends.phyUser->getPhyModeProvider()->getPhyMode(phyModeId);
        phyMode.setNumberOfSpatialStreams(ss);

        MESSAGE_BEGIN(NORMAL, *logger, m, "RA unknown PER");
        m << " to receiver " << receiver;
        m << " transmissions: " << numTransmissions;
        m << " numTx: " << numTx;
        m << " numRx: " << numRx;
        m << " use " << phyMode;
        MESSAGE_END();

        return(phyMode);
    }

    double curPER = per->getPER(receiver);
    int nextPhyModeId = curPhyModeId;
    unsigned int nextSpatialStreams = curSpatialStreams;


    if(curPER > 0.25)
    {
        // loose more than 1/4 of all frames -> go down
        if(curPhyModeId > friends.phyUser->getPhyModeProvider()->getLowestId())
        {
            nextPhyModeId = curPhyModeId-1;
        }
        else
        {
            // no more going down possible, reduce num antennas
            if(curSpatialStreams > 1)
            {
                nextSpatialStreams = curSpatialStreams - 1;
                // increase phy mode again
                nextPhyModeId += 3;
            }
        }
    }

    if(curPER < 0.01)
    {
        // nearly all frames are successful -> go up
        if(curPhyModeId < friends.phyUser->getPhyModeProvider()->getHighestId())
        {
            nextPhyModeId = curPhyModeId+1;
        }
        else
        {
            // no more going up possible, increase num antennas
            if(curSpatialStreams < maxNumSS)
            {
                nextSpatialStreams = curSpatialStreams + 1;
                // decrease phy mode
                nextPhyModeId -= 3;
            }
        }
    }

    assure(nextPhyModeId >= friends.phyUser->getPhyModeProvider()->getLowestId(),
           "Invalid nextPhyModeId");
    assure(nextSpatialStreams >= 1, "Invalid nextSpatialStreams");

    wifimac::convergence::PhyMode nextPhyMode = friends.phyUser->getPhyModeProvider()->getPhyMode(nextPhyModeId);
    nextPhyMode.setNumberOfSpatialStreams(nextSpatialStreams);

    if(nextPhyMode != curPhyMode)
    {
        MESSAGE_BEGIN(NORMAL, *logger, m, "RA");
        m << " to receiver " << receiver;
        m << " per: " << per->getPER(receiver);
        m << " numTx: " << numTx;
        m << " numRx: " << numRx;
        m << " going from " << curPhyMode;
        m << " to " << nextPhyMode;
        MESSAGE_END();

        per->reset(receiver);
    }

    curPhyModeId = nextPhyModeId;
    curSpatialStreams = nextSpatialStreams;

    return(nextPhyMode);
}
