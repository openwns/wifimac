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

#ifndef WIFIMAC_LOWERMAC_RATEADAPTATIONSTRATEGIES_ARFWITHMIMO_HPP
#define WIFIMAC_LOWERMAC_RATEADAPTATIONSTRATEGIES_ARFWITHMIMO_HPP

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/IRateAdaptationStrategy.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/draftn/SINRwithMIMOInformationBase.hpp>

#include <WNS/ldk/Key.hpp>
#include <WNS/distribution/Uniform.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/events/CanTimeout.hpp>

namespace wifimac { namespace lowerMAC { namespace rateAdaptationStrategies {

    /**
	 * @brief The ARFWITHMIMO Rate Adpation tries to find the maximum MCS with
     *   a packet error rate below a given value.
     *
     * ARFWITHMIMO RA works only using the statistics of received ACKs as a
     * reply to the used MCS. Hence, it slowly increases the rate by selecting
     * high-rate MCSs, until the PER exceeds a given value. Then, it stays at
     * the next lower MCS until the percieved PER changes again.
	 */
    class ARFwithMIMO:
        public IRateAdaptationStrategy,
        public wns::events::CanTimeout
    {
    public:
        ARFwithMIMO(
            const wns::pyconfig::View& config_,
            wifimac::management::PERInformationBase* _per,
            wifimac::management::SINRInformationBase* _sinr,
            wifimac::lowerMAC::Manager* _manager,
            wifimac::convergence::PhyUser* _phyUser,
            wns::logger::Logger* _logger);

        wifimac::convergence::PhyMode
        getPhyMode(const wns::service::dll::UnicastAddress receiver,
                   size_t numTransmissions) const;

        wifimac::convergence::PhyMode
        getPhyMode(const wns::service::dll::UnicastAddress receiver,
                   size_t numTransmissions,
                   const wns::Ratio lqm) const;

        void
        setCurrentPhyMode(const wns::service::dll::UnicastAddress receiver,
                          wifimac::convergence::PhyMode pm);

    private:
        wifimac::convergence::PhyMode
        getPhyMode(const wns::service::dll::UnicastAddress receiver,
                   size_t numTransmissions,
                   const std::vector<wifimac::convergence::PhyMode>& myPMList) const;

        void
        onTimeout();

        void
        reducePhyMode();

        void
        increasePhyMode();

        wifimac::management::PERInformationBase* per;
        const wns::simulator::Time arfTimer;
        const bool exponentialBackoff;
        const int initialSuccessThreshold;
        const int maxSuccessThreshold;
        int successThreshold;
        bool probePacket;

        struct Friends
        {
            wifimac::convergence::PhyUser* phyUser;
            wifimac::lowerMAC::Manager* manager;
        } friends;

        wns::logger::Logger* logger;

        wns::service::dll::UnicastAddress myReceiver;
        std::vector<wifimac::convergence::PhyMode> pmList;
        int curPhyModeId;
        bool knowsReceiver;
    };
}}}

#endif
