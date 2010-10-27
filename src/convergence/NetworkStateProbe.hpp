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

#ifndef WIFIMAC_CONVERGENCE_NETWORKSTATEPROBE_HPP
#define WIFIMAC_CONVERGENCE_NETWORKSTATEPROBE_HPP

#include <WIFIMAC/convergence/TxDurationSetter.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>
#include <WNS/ldk/Command.hpp>
#include <WNS/logger/Logger.hpp>

#include <WNS/ldk/probe/Probe.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>
#include <WNS/probe/bus/ContextProvider.hpp>
#include <WNS/events/CanTimeout.hpp>

namespace wifimac { namespace convergence {

    class NetworkStateProbeCommand :
        public wns::ldk::EmptyCommand
    {
    };

    /**
     * @brief Probing the network state from a local point of view
     */

    class NetworkStateProbe:
        public wns::ldk::fu::Plain<NetworkStateProbe, NetworkStateProbeCommand>,
        public wns::ldk::Processor<NetworkStateProbe>,
        public wns::ldk::probe::Probe,
        public wns::events::CanTimeout
    {

    public:
        NetworkStateProbe(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
        virtual ~NetworkStateProbe();

    private:
        /** @brief Processor Interface Implementation */
        void
        processIncoming(const wns::ldk::CompoundPtr& compound);
        void
        processOutgoing(const wns::ldk::CompoundPtr& compound);

        /**
         * @brief canTimeoutInterface to probe tx frames at the end of the
         *  transmission
         */
        void onTimeout();

        wns::logger::Logger logger;

        /** @brief Name of the command that holds the frame duration */
        std::string txDurationProviderCommandName;

        /** @brief Probe the (local) network state */
        wns::probe::bus::ContextCollectorPtr localNetworkState;

        /** @brief Distinguish Rx and Tx */
        wns::probe::bus::contextprovider::Variable* isTx;

        wns::ldk::CompoundPtr curTxCompound;
        wns::simulator::Time curFrameTxDuration;
    };

} // ns convergence
} // ns wifimac

#endif //WIFIMAC_CONVERGENCER_NETWORKSTATEPROBE_HPP
