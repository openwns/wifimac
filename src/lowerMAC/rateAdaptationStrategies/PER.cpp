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

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/PER.hpp>
#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>

#include <algorithm>

using namespace wifimac::lowerMAC::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(PER, IRateAdaptationStrategy, "PER", IRateAdaptationStrategyCreator);

PER::PER(
    const wns::pyconfig::View& _config,
    wns::service::dll::UnicastAddress _receiver,
    wifimac::management::PERInformationBase* _per,
    wifimac::management::SINRInformationBase* _sinr,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    IRateAdaptationStrategy(_config, _receiver, _per, _sinr, _manager, _phyUser, _logger),
    myReceiver(_receiver),
    per(_per),
    perForGoingDown(_config.get<double>("perForGoingDown")),
    perForGoingUp(_config.get<double>("perForGoingUp")),
    logger(_logger)
{
    friends.phyUser = _phyUser;
    curPhyMode = _config.getView("initialPhyMode");
}

wifimac::convergence::PhyMode
PER::getPhyMode(size_t numTransmissions, const wns::Ratio /*lqm*/) const
{
    return(this->getPhyMode(numTransmissions));
}

wifimac::convergence::PhyMode
PER::getPhyMode(size_t numTransmissions) const
{
    if(not per->knowsPER(myReceiver))
    {
        assure(numTransmissions >= 1, "Must have at least one transmission");

        // if no information about the PER is available (due to a recent change),
        // we take the current phyMode 
        return(curPhyMode);
    }

    double curPER = per->getPER(myReceiver);
    wifimac::convergence::PhyMode pm = curPhyMode;

    if(curPER > perForGoingDown)
    {
        // loose more than perForGoingDown of all frames -> go down
        friends.phyUser->getPhyModeProvider()->mcsDown(pm);
    }
    if(curPER < perForGoingUp)
    {
        // nearly all frames are successful -> go up
        friends.phyUser->getPhyModeProvider()->mcsUp(pm);
    }

    return(pm);
}

void
PER::setCurrentPhyMode(wifimac::convergence::PhyMode pm)
{
    if(curPhyMode != pm)
    {
        MESSAGE_SINGLE(NORMAL, *logger, "RA going from MCS "<< curPhyMode << " to " << pm);
        per->reset(myReceiver);
        curPhyMode = pm;
    }
}
