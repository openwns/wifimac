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

#ifndef WIFIMAC_LOWERMAC_RATEADAPTATIONSTRATEGIES_SINR_HPP
#define WIFIMAC_LOWERMAC_RATEADAPTATIONSTRATEGIES_SINR_HPP

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/ARF.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/ldk/Key.hpp>
#include <WNS/distribution/Uniform.hpp>
#include <WNS/logger/Logger.hpp>

namespace wifimac { namespace lowerMAC { namespace rateAdaptationStrategies {

    /**
	 * @brief The SINR-based Rate Adaptation (RA) selects the MCS based on the
	 * knowledge of received link quality indicators.
     *
     * In contrast to the PER RA, the SINR-based RA gets direct
     * feedback about the link quality from the peer of the link. Hence, it can
     * directly select the matching MCS for the indicated (averaged) link
     * quality.
	 */
    class SINR:
        public ARF
    {
    public:
        SINR(
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

        void
        setCurrentPhyMode(wifimac::convergence::PhyMode pm);

    private:
        wifimac::management::PERInformationBase* per;
        wifimac::management::SINRInformationBase* sinr;

        struct Friends
        {
            wifimac::convergence::PhyUser* phyUser;
        } friends;

        const wns::service::dll::UnicastAddress myReceiver;
        const double retransmissionLQMReduction;

        wns::logger::Logger* logger;
    };
}}}

#endif
