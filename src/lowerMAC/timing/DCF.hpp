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

#ifndef WIFIMAC_LOWERMAC_TIMING_DCF_HPP
#define WIFIMAC_LOWERMAC_TIMING_DCF_HPP

#include <WIFIMAC/lowerMAC/timing/Backoff.hpp>
#include <WIFIMAC/convergence/IChannelState.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>
#include <WNS/ldk/Command.hpp>


namespace wifimac { namespace lowerMAC { namespace timing {

	/** @brief Distributed Coordination Function */
	class DCF:
		public wns::ldk::fu::Plain<DCF, wns::ldk::EmptyCommand>,
        public wns::ldk::Delayed<DCF>,
		public virtual BackoffObserver
	{

	public:

		DCF(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

		virtual
		~DCF();

        /// Delayed interface realization
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);
        bool hasCapacity() const;
        const wns::ldk::CompoundPtr hasSomethingToSend() const;
        wns::ldk::CompoundPtr getSomethingToSend();

        virtual void onFUNCreated();

	private:

		/** @brief BackoffObserver interface for the transmission of data frames */
		virtual void backoffExpired();

        /** @brief Storage of the current frame that waits for tx permission */
		wns::ldk::CompoundPtr currentFrame;

		const std::string csName;
        const std::string arqCommandName;

		/** @brief The backoff instance */
		Backoff backoff;

        /** @brief indicates that transmission is permitted */
        bool sendNow;

		wns::logger::Logger logger;
	};
} // timing
} // lowerMAC
} // wifimac

#endif // WIFIMAC_LOWERMAC_TIMING_DCF_HPP
