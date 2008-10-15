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

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/SINR.hpp>
#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>

using namespace wifimac::lowerMAC::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(SINR, IRateAdaptationStrategy, "SINR", IRateAdaptationStrategyCreator);

SINR::SINR(
    wifimac::management::PERInformationBase* _per,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    IRateAdaptationStrategy(_per, _manager, _phyUser, _logger),
    per(_per),
    logger(_logger)
{
    friends.phyUser = _phyUser;
    curPhyModeId = friends.phyUser->getPhyModeProvider()->getLowestId();
}

wifimac::convergence::PhyMode
SINR::getPhyMode(const wns::service::dll::UnicastAddress /*receiver*/, const wns::Ratio lqm)
{
    MESSAGE_SINGLE(NORMAL, *logger, "RA getPhyMode with lqm " << lqm << ", suggested phyMode " << friends.phyUser->getPhyModeProvider()->getPhyMode(lqm));
    return(friends.phyUser->getPhyModeProvider()->getPhyMode(lqm));
}

wifimac::convergence::PhyMode
SINR::getPhyMode(const wns::service::dll::UnicastAddress receiver)
{
    if(not per->knowsPER(receiver))
    {
        return(friends.phyUser->getPhyModeProvider()->getPhyMode(curPhyModeId));
    }

    double curPER = per->getPER(receiver);
    int nextPhyModeId = curPhyModeId;


    if(curPER > 0.25)
    {
        // loose more than 1/4 of all frames -> go down
        if(curPhyModeId > friends.phyUser->getPhyModeProvider()->getLowestId())
        {
            nextPhyModeId = curPhyModeId-1;
        }
    }
    if(curPER < 0.05)
    {
        // nearly all frames are successful -> go up
        if(curPhyModeId < friends.phyUser->getPhyModeProvider()->getHighestId())
        {
            nextPhyModeId = curPhyModeId+1;
        }
    }

    if(curPhyModeId != nextPhyModeId)
    {
        MESSAGE_SINGLE(NORMAL, *logger, "RA going from MCS "<< curPhyModeId << " to " << nextPhyModeId);
        per->reset(receiver);
    }
    curPhyModeId = nextPhyModeId;
    wifimac::convergence::PhyMode mcs = friends.phyUser->getPhyModeProvider()->getPhyMode(curPhyModeId);

    return(mcs);
}
