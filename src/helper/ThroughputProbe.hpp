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
	public:
		HopContextWindowProbe(wns::ldk::fun::FUN* fuNet, const wns::pyconfig::View& config);
		virtual ~HopContextWindowProbe();

		// Processor interface
		virtual void processIncoming(const wns::ldk::CompoundPtr& compound);

	private:
		virtual void onFUNCreated();

		/** @brief Add sample for incomming bits
		 *
		 *  The sample is sorted into the probe with the given number of hops,
		 *  this sample is the first one the probe & putter is created
		 */
		void
		putIntoBitsIncomming(unsigned int numHops, double value);

		/** @brief Add sample for aggregated bits
		 *
		 *  The sample is sorted into the probe with the given number of hops,
		 *  this sample is the first one the probe & putter is created
		 */
		void
		putIntoBitsAggregated(unsigned int numHops, double value);

		/** @brief Add sample for incomming compounds
		 *
		 *  The sample is sorted into the probe with the given number of hops,
		 *  this sample is the first one the probe & putter is created
		 */
		void
		putIntoCompoundsIncomming(unsigned int numHops, double value);

		/** @brief Add sample for aggregated compounds
		 *
		 *  The sample is sorted into the probe with the given number of hops,
		 *  this sample is the first one the probe & putter is created
		 */
		void
		putIntoCompoundsAggregated(unsigned int numHops, double value);

		
		/** @brief Helper for all putInto* functions*/
		wns::SlidingWindow*
		getProbe(SlidingWindowMap& probeHolder, PutterMap& putterHolder, std::string id, unsigned int numHops);

		/** @brief Periodically storing of the slidingWindow results */
		void
		periodically();

		/** @brief Helper for periodically: store all probes in the given
		 *   sliding windows into the putters
		 */
		void
		storeProbes(SlidingWindowMap& probeHolder, PutterMap& putterHolder);

		/** @brief Access the number of hops in the forwarding command */
		wns::ldk::CommandReaderInterface* forwardingReader;

		wns::pyconfig::View config_;
		const simTimeType windowSize;

		/** @brief Store all putter in a registry, sorted by hop-count, created
		 *	on-demand */
		typedef wns::container::Registry<int, wns::probe::bus::ContextCollectorPtr, wns::container::registry::DeleteOnErase> PutterMap;
		PutterMap bitsIncomingPutterHolder;
		PutterMap bitsAggregatedPutterHolder;
		PutterMap compoundsIncomingPutterHolder;
		PutterMap compoundsAggregatedPutterHolder;

        /** @brief Store all probes(sliding windows) in a registry, sorted by hop-count, created
		 *	on-demand */
		typedef wns::container::Registry<int, wns::SlidingWindow*, wns::container::registry::DeleteOnErase> slidingWindowMap;
		slidingWindowMap bitsIncomingProbeHolder;
		slidingWindowMap bitsAggregatedProbeHolder;
		slidingWindowMap compoundsIncomingProbeHolder;
		slidingWindowMap compoundsAggregatedProbeHolder;

		/** @brief Required to differentiate between probes in different
		 *	transceivers of the same node */
		struct Friends
		{
			wifimac::multiradio::Manager* manager;
		} friends;
	};

} // Helper
} // WiFiMac

#endif 
