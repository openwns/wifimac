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

#ifndef WIFIMAC_DRAFTN_RATEADAPTATIONSTRATEGIES_SINRWITHMIMO_HPP
#define WIFIMAC_DRAFTN_RATEADAPTATIONSTRATEGIES_SINRWITHMIMO_HPP

#include <WIFIMAC/draftn/rateAdaptationStrategies/PERwithMIMO.hpp>
#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/SINR.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/ldk/Key.hpp>
#include <WNS/distribution/Uniform.hpp>
#include <WNS/logger/Logger.hpp>

namespace wifimac { namespace draftn { namespace rateAdaptationStrategies {

    /**
	 * @brief The SINR-based Rate Adaptation (RA) selects the MCS/antenna based
	 * on the knowledge of received link quality indicators.
     *
     * In contrast to the PER RA, the SINR-based RA gets direct
     * feedback about the link quality from the peer of the link. Hence, it can
     * directly select the matching MCS and number of antennas for the indicated
     * (averaged) link quality.
	 */
    class SINRwithMIMO:
        public PERwithMIMO
    {
    public:
        SINRwithMIMO(
            const wns::pyconfig::View& _config,
            wns::service::dll::UnicastAddress _receiver,
            wifimac::management::PERInformationBase* _per,
            wifimac::management::SINRInformationBase* _sinr,
            wifimac::lowerMAC::Manager* _manager,
            wifimac::convergence::PhyUser* _phyUser,
            wns::logger::Logger* _logger);

        wifimac::convergence::PhyMode
        getPhyMode(size_t numTransmissions) const;

        wifimac::convergence::PhyMode
        getPhyMode(size_t numTransmissions,
                   const wns::Ratio lqm) const;

    private:
        struct Friends
        {
            wifimac::convergence::PhyUser* phyUser;
            wifimac::lowerMAC::Manager* manager;
        } friends;

        wifimac::draftn::SINRwithMIMOInformationBase* sinr;
        wifimac::lowerMAC::rateAdaptationStrategies::SINR singleStreamRA;
        const double retransmissionLQMReduction;
        const wns::service::dll::UnicastAddress myReceiver;

        wns::logger::Logger* logger;

        unsigned int curSpatialStreams;
    };
}}}

#endif
