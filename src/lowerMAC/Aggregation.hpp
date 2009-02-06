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

#ifndef WIFIMAC_LOWERMAC_AGGREGATION_HPP
#define WIFIMAC_LOWERMAC_AGGREGATION_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/ldk/concatenation/Concatenation.hpp>
#include <WNS/events/CanTimeout.hpp>
#include <WNS/ldk/probe/Probe.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>

namespace wifimac { namespace lowerMAC {

    class Aggregation:
        public wns::ldk::concatenation::Concatenation,
        public wns::events::CanTimeout,
        public wns::ldk::probe::Probe
    {
    public:
        Aggregation(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        void processOutgoing(const wns::ldk::CompoundPtr& compound);
        const wns::ldk::CompoundPtr hasSomethingToSend() const;
        wns::ldk::CompoundPtr getSomethingToSend();

        void onFUNCreated();

       /// CanTimeout interface realization
        void onTimeout();

    private:
        const std::string managerName;
        const bool impatientTransmission;
        const wns::simulator::Time maxDelay;
        bool sendNow;
        wns::service::dll::UnicastAddress currentReceiver;

        wns::probe::bus::ContextCollectorPtr aggregationSizeFrames;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;
    };


} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_AGGREGATION_HPP
