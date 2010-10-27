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

#ifndef WIFIMAC_HELPER_DESTINATIONSORTEDWINDOWPROBE_HPP
#define WIFIMAC_HELPER_DESTINATIONSORTEDWINDOWPROBE_HPP

#include <WNS/ldk/probe/bus/Window.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>
#include <WNS/SlidingWindow.hpp>
#include <WNS/ldk/CommandReaderInterface.hpp>
#include <WNS/container/Registry.hpp>

#include <vector>

namespace wifimac { namespace helper {

    /**
	 * @brief FunctionalUnit to probe windowing throughputs that include the
	 * destination of the frame as context
	 *
	 * This is a derivate of the wns::ldk::probe::Window
	 *
	 */
    class DestinationSortedWindowProbe:
        public wns::ldk::probe::bus::Window
    {
        typedef wns::container::Registry<unsigned int, wns::SlidingWindow*, wns::container::registry::DeleteOnErase> SlidingWindowMap;

    public:
        DestinationSortedWindowProbe(wns::ldk::fun::FUN* fuNet, const wns::pyconfig::View& config);
        virtual ~DestinationSortedWindowProbe();

        // Processor interface
        virtual void processIncoming(const wns::ldk::CompoundPtr& compound);
        virtual void processOutgoing(const wns::ldk::CompoundPtr& compound);

        void
        putAggregatedProbe(unsigned int adr,
                           double value);

    private:
        virtual void onFUNCreated();

        void
        putProbe(SlidingWindowMap& probeHolder,
                 unsigned int adr,
                 double value);

        /** @brief Periodically storing of the slidingWindow results */
        void
        periodically();

        /** @brief Helper for periodically: store all probes in the given
         *   sliding windows into the putters
         */
        void
        storeProbes(SlidingWindowMap& probeHolder, wns::probe::bus::ContextCollectorPtr& putter);

        /** @brief Access the source and destination*/
        wns::ldk::CommandReaderInterface* ucReader;

        wns::pyconfig::View config;
        const simTimeType windowSize;
        const std::string ucCommandName;

        /** @brief Store all putter in a registry, sorted by address, created
         *	on-demand */
        wns::probe::bus::ContextCollectorPtr bitsIncoming;
        wns::probe::bus::ContextCollectorPtr bitsAggregated;
        wns::probe::bus::ContextCollectorPtr bitsOutgoing;
        wns::probe::bus::ContextCollectorPtr relativeGoodput;

        /** @brief Store all probes(sliding windows) in a registry, sorted by address, created
         *	on-demand */
        SlidingWindowMap bitsIncomingProbeHolder;
        SlidingWindowMap bitsAggregatedProbeHolder;
        SlidingWindowMap bitsOutgoingProbeHolder;
    };

} // Helper
} // WiFiMac

#endif
