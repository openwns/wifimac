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
    Opportunistic(_per, _manager, _phyUser, _logger),
    per(_per),
    logger(_logger)
{
    friends.phyUser = _phyUser;
}

wifimac::convergence::PhyMode
SINR::getPhyMode(const wns::service::dll::UnicastAddress /*receiver*/, size_t numTransmissions, const wns::Ratio lqm)
{
    // Reduce lqm by 3dB for every retransmission
    wns::Ratio myLQM = wns::Ratio::from_dB(lqm.get_dB() - (numTransmissions-1)*3.0);
    MESSAGE_SINGLE(NORMAL, *logger, "RA getPhyMode with lqm " << lqm << " and " << numTransmissions << " transmissions, suggested phyMode " << friends.phyUser->getPhyModeProvider()->getPhyMode(myLQM));
    return(friends.phyUser->getPhyModeProvider()->getPhyMode(myLQM));
}

wifimac::convergence::PhyMode
SINR::getPhyMode(const wns::service::dll::UnicastAddress receiver, size_t numTransmissions)
{
    return(Opportunistic::getPhyMode(receiver, numTransmissions));
}
