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

using namespace wifimac::lowerMAC::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(SINR, IRateAdaptationStrategy, "SINR", IRateAdaptationStrategyCreator);

SINR::SINR(
    const wns::pyconfig::View& _config,
    wns::service::dll::UnicastAddress receiver,
    wifimac::management::PERInformationBase* _per,
    wifimac::management::SINRInformationBase* _sinr,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    ARF(_config, receiver, _per, _sinr, _manager, _phyUser, _logger),
    per(_per),
    sinr(_sinr),
    myReceiver(receiver),
    retransmissionLQMReduction(_config.get<double>("retransmissionLQMReduction")),
    logger(_logger)
{
    friends.phyUser = _phyUser;
}

wifimac::convergence::PhyMode
SINR::getPhyMode(size_t numTransmissions, const wns::Ratio lqm) const
{
    // Reduce lqm by retransmissionLQMReduction dB for every retransmission
    wns::Ratio myLQM = wns::Ratio::from_dB(lqm.get_dB() - (numTransmissions-1)*retransmissionLQMReduction);
    wifimac::convergence::PhyMode pm = friends.phyUser->getPhyModeProvider()->getDefaultPhyMode();
    pm.setMCS(friends.phyUser->getPhyModeProvider()->getMCS(myLQM));
    MESSAGE_SINGLE(NORMAL, *logger, "RA getPhyMode with lqm " << lqm << " and " << numTransmissions << " transmissions, suggested phyMode " << pm);
    return(pm);
}

wifimac::convergence::PhyMode
SINR::getPhyMode(size_t numTransmissions) const
{
    if(sinr->knowsPeerSINR(myReceiver))
    {
        return(getPhyMode(numTransmissions, sinr->getPeerSINR(myReceiver)));
    }
    else
    {
        return(ARF::getPhyMode(numTransmissions));
    }
}

void
SINR::setCurrentPhyMode(wifimac::convergence::PhyMode pm)
{
    if(not sinr->knowsPeerSINR(myReceiver))
    {
        // no SINR known, phyMode setting is handled by ARF
        ARF::setCurrentPhyMode(pm);
    }
    // otherwise do nothing, no internal state required
}
