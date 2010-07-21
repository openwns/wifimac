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

#ifndef WIFIMAC_DRAFTN_RATEADAPTATIONSTRATEGIES_OPPORTUNISTICWITHMIMO_HPP
#define WIFIMAC_DRAFTN_RATEADAPTATIONSTRATEGIES_OPPORTUNISTICWITHMIMO_HPP

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/IRateAdaptationStrategy.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/draftn/SINRwithMIMOInformationBase.hpp>

#include <WNS/ldk/Key.hpp>
#include <WNS/distribution/Uniform.hpp>
#include <WNS/logger/Logger.hpp>

namespace wifimac { namespace draftn { namespace rateAdaptationStrategies {

    /**
	 * @brief The Opportunisitc Rate Adpation tries to find the maximum rate
     *   (using MCSs and number of antennas) with a packet error rate below a
     *   given value.
     *
     * Opportunistic RA works only using the statistics of received ACKs as a
     * reply to the used MCS and number of antennas. Hence, it slowly increases
     * the rate by selecting high-rate MCSs (and switching the number of
     * antennas, if possible), until the PER exceeds a given value. Then, it
     * stays at the next lower MCS/antenna combination until the percieved PER
     * changes again.
	 */
    class OpportunisticwithMIMO:
        public wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategy
    {
    public:
        OpportunisticwithMIMO(
            const wns::pyconfig::View& config_,
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
        void
        reducePhyMode(wifimac::convergence::PhyMode& pm, unsigned int maxNumSS) const;

        void
        increasePhyMode(wifimac::convergence::PhyMode& pm, unsigned int maxNumSS) const;

        wifimac::management::PERInformationBase* per;

        struct Friends
        {
            wifimac::convergence::PhyUser* phyUser;
            wifimac::lowerMAC::Manager* manager;
        } friends;

        const double perForGoingDown;
        const double perForGoingUp;
        const unsigned int phyModeIncreaseOnAntennaDecrease;
        const unsigned int phyModeDecreaseOnAntennaIncrease;
        const wns::service::dll::UnicastAddress myReceiver;

        wns::logger::Logger* logger;

        wifimac::convergence::PhyMode curPhyMode;
    };
}}}

#endif
