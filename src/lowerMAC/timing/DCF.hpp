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
#include <WIFIMAC/convergence/IRxStartEnd.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Command.hpp>


namespace wifimac { namespace lowerMAC { namespace timing {

    /** @brief Distributed Coordination Function
     *
     *  Implements the known CSMA/CA using exponential backoff. FU does _not_
     *  store any compounds, but uses the doIsAccepting to trigger the backoff:
     *  1. Upper FU calls doIsAccepting -> backoff instance is asked for
     *  permission
     *  2a. (Post-)backoff has run down: Return true, packet is send
     *  2b. Backoff has not finished: Return false
     *  3b. Backoff calls DCF back when finished
     *  4b. DCF wakes upper FU up, re-start at 1
     */
    class DCF:
        public wns::ldk::fu::Plain<DCF, wns::ldk::EmptyCommand>,
        public virtual BackoffObserver
    {

    public:

        DCF(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        virtual
        ~DCF();

        /// compound Handler Interface
        void doSendData(const wns::ldk::CompoundPtr& compound);
        void doOnData(const wns::ldk::CompoundPtr& compound);
        bool doIsAccepting(const wns::ldk::CompoundPtr& compound) const;
        void doWakeup();

        virtual void onFUNCreated();

        /// notifying observers every time the backoff has finished, wether or not a transmission is waiting
        void
        registerEOBObserver(BackoffObserver* observer) {backoff.registerEOBObserver(observer);}

        /// time the backoff will be finished at (when called during AIFS the maximum waiting time will be used)
        /// wns::simulator::Time() if backoff hasn't started, current time when finished and idle
        wns::simulator::Time
        backoffFinishedAt() {return backoff.finishedAt();}

    private:

        /** @brief BackoffObserver interface for the transmission of data frames */
        virtual void backoffExpired();

        const std::string csName;
        const std::string rxStartEndName;
        const std::string arqCommandName;
        const bool backoffDisabled;

        /** @brief The backoff instance */
        mutable Backoff backoff;

        /** @brief indicates that transmission is permitted */
        mutable bool sendNow;

        wns::logger::Logger logger;
    };
} // timing
} // lowerMAC
} // wifimac

#endif // WIFIMAC_LOWERMAC_TIMING_DCF_HPP
