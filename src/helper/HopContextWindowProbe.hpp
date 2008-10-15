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

#ifndef WIFIMAC_HELPER_HOPCONTEXTWINDOWPROBE_HPP
#define WIFIMAC_HELPER_HOPCONTEXTWINDOWPROBE_HPP

#include <WNS/ldk/probe/bus/Window.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>
#include <WNS/SlidingWindow.hpp>
#include <WNS/ldk/CommandReaderInterface.hpp>
#include <WNS/container/Registry.hpp>

#include <vector>

namespace wifimac { namespace helper {

	/**
	 * @brief FunctionalUnit to probe windowing throughputs that include the hopcount.
	 *
	 * This is a derivate of the wns::ldk::probe::Window
	 *
	 */
	class HopContextWindowProbe:
		public wns::ldk::probe::bus::Window
	{
		typedef wns::container::Registry<int, wns::SlidingWindow*, wns::container::registry::DeleteOnErase> SlidingWindowMap;

	public:
		HopContextWindowProbe(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
		virtual ~HopContextWindowProbe();

		// Processor interface
		virtual void processIncoming(const wns::ldk::CompoundPtr& compound);

	private:
		virtual void onFUNCreated();

		/** @brief put a probe sample into the (hop-sorted)
		 * slidingWindowRegistry, create new entry on-demand */
		void
		putProbe(SlidingWindowMap& probeHolder, unsigned int numHops, double value);

		/** @brief Periodically storing of the slidingWindow results */
		void
		periodically();

		/** @brief Helper for periodically: store all probes in the given
		 *   sliding windows into the putters */
		void
		storeProbes(SlidingWindowMap& probeHolder, wns::probe::bus::ContextCollectorPtr& putter);

		wns::pyconfig::View config_;

		/** @brief Access the number of hops in the forwarding command */
		wns::ldk::CommandReaderInterface* forwardingReader;

		/** @brief One common window size for all probes */
		const simTimeType windowSize;

		/** @brief Store all putter in a registry, sorted by hop-count, created
		 *	on-demand */
		wns::probe::bus::ContextCollectorPtr hopCountedBitsIncoming;
		wns::probe::bus::ContextCollectorPtr hopCountedBitsAggregated;
		wns::probe::bus::ContextCollectorPtr hopCountedCompoundsIncoming;
		wns::probe::bus::ContextCollectorPtr hopCountedCompoundsAggregated;

        /** @brief Store all probes(sliding windows) in a registry, sorted by hop-count, created
		 *	on-demand */
		SlidingWindowMap bitsIncomingProbeHolder;
		SlidingWindowMap bitsAggregatedProbeHolder;
		SlidingWindowMap compoundsIncomingProbeHolder;
		SlidingWindowMap compoundsAggregatedProbeHolder;

	};

} // Helper
} // WiFiMac

#endif 
