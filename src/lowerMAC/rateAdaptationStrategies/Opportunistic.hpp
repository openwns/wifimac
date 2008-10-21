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

#ifndef WIFIMAC_LOWERMAC_RATEADAPTATIONSTRATEGIES_OPPORTUNISTIC_HPP
#define WIFIMAC_LOWERMAC_RATEADAPTATIONSTRATEGIES_OPPORTUNISTIC_HPP

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/IRateAdaptationStrategy.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/ldk/Key.hpp>
#include <WNS/distribution/Uniform.hpp>
#include <WNS/logger/Logger.hpp>

namespace wifimac { namespace lowerMAC { namespace rateAdaptationStrategies {

    /**
	 * @brief The Rate Adpation (RA) tries to select the optimal Modulation- and Coding-Scheme (MCS)
	 *    for the upcomming transmission.
	 */

    class Opportunistic:
        public IRateAdaptationStrategy
    {
    public:
        Opportunistic(
            wifimac::management::PERInformationBase* _per,
            wifimac::lowerMAC::Manager* _manager,
            wifimac::convergence::PhyUser* _phyUser,
            wns::logger::Logger* _logger);

        wifimac::convergence::PhyMode
        getPhyMode(const wns::service::dll::UnicastAddress receiver,
                   size_t numTransmissions);

        wifimac::convergence::PhyMode
        getPhyMode(const wns::service::dll::UnicastAddress receiver,
                   size_t numTransmissions,
                   const wns::Ratio lqm);

    private:
        wifimac::management::PERInformationBase* per;

        struct Friends
        {
            wifimac::convergence::PhyUser* phyUser;
        } friends;

        wns::logger::Logger* logger;

        int curPhyModeId;
    };
}}}

#endif
