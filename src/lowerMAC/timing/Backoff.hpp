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

#ifndef WIFIMAC_LOWERMAC_TIMING_BACKOFF_HPP
#define WIFIMAC_LOWERMAC_TIMING_BACKOFF_HPP

#include <WIFIMAC/convergence/IChannelState.hpp>

#include <WNS/Observer.hpp>

#include <WNS/events/MemberFunction.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/distribution/Uniform.hpp>
#include <WNS/simulator/Time.hpp>
#include <WNS/events/CanTimeout.hpp>

namespace wifimac { namespace lowerMAC { namespace timing {

	class BackoffObserver
	{
	public:
		virtual
		~BackoffObserver()
		{}

		virtual void backoffExpired() = 0;
	};

	/**
	 * @brief IEEE 802.11 DCF style backoff, including post-backoff after transmissions
	 *
	 * Essentially, the backoff in IEEE is very simple: Assure that between every channel busy->idle transition
	 * and the node's transmission a backoff is counted to zero.
	 */
	class Backoff :
		public wns::events::CanTimeout,
        public wns::Observer<wifimac::convergence::IChannelState>
	{
	public:
		explicit
		Backoff(
			BackoffObserver* backoffObserver,
			const wns::pyconfig::View& config);

		virtual ~Backoff();

		/** @brief Transmission request by the scheduler */
		virtual bool
        transmissionRequest(int transmissionCounter);

		/** @brief Indicates a channel state transition idle->busy */
		virtual void
        onChannelBusy();

		/** @brief Indicates a channel state transition busy->idle */
		virtual void
        onChannelIdle();

		int getCurrentCW() const
		{
			return(cw);
		}

	private:

		void startNewBackoffCountdown(wns::simulator::Time ifsDuration);

		/** @brief implementation of CanTimeout interface */
		virtual void onTimeout();

		void waitForTimer(const wns::simulator::Time& waitDuration);

		BackoffObserver* backoffObserver;

		const wns::simulator::Time slotDuration;
		const wns::simulator::Time aifsDuration;

		bool backoffFinished;
		bool transmissionWaiting;
		bool duringAIFS;

		const int cwMin;
		const int cwMax;
		int cw;
		wns::distribution::Uniform uniform;
		wns::logger::Logger logger;
        bool channelIsBusy;

	protected:
		// For testing purpose this methods and variable is protected and may be
		// set by a special version of the backoff
		int counter;
	};
} // timing
} // lowerMAC
} // wifimac

#endif //  WIFIMAC_SCHEDULER_BACKOFF_HPP
