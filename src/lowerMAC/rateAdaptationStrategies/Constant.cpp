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

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/Constant.hpp>

using namespace wifimac::lowerMAC::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(Constant, IRateAdaptationStrategy, "Constant", IRateAdaptationStrategyCreator);

Constant::Constant(const wns::pyconfig::View& _config,
                   wns::service::dll::UnicastAddress _receiver,
                   wifimac::management::PERInformationBase* _per,
                   wifimac::management::SINRInformationBase* _sinr,
                   wifimac::lowerMAC::Manager* _manager,
                   wifimac::convergence::PhyUser* _phyUser,
                   wns::logger::Logger* _logger):
    IRateAdaptationStrategy(_config, _receiver, _per, _sinr, _manager, _phyUser, _logger),
    myPM(_config.getView("phyMode"))
{
    friends.phyUser = _phyUser;
}

wifimac::convergence::PhyMode
Constant::getPhyMode(size_t /*numTransmissions*/) const
{
    return(this->myPM);
}

wifimac::convergence::PhyMode
Constant::getPhyMode(size_t /*numTransmissions*/, const wns::Ratio /*lqm*/) const
{
    return(this->myPM);
}

void
Constant::setCurrentPhyMode(wifimac::convergence::PhyMode pm)
{
     // do nothing, be CONSTANT!
};
