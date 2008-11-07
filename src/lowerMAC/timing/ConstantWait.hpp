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

#ifndef WIFIMAC_LOWERMAC_TIMING_CONSTANTWAIT_HPP
#define WIFIMAC_LOWERMAC_TIMING_CONSTANTWAIT_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>
#include <WNS/ldk/Command.hpp>

#include <WNS/simulator/Time.hpp>
#include <WNS/events/CanTimeout.hpp>

namespace wifimac { namespace lowerMAC { namespace timing {

    /** @brief Channel Access for transmissions with constant waiting time, i.e. ACKs */
    class ConstantWait:
        public wns::ldk::fu::Plain<ConstantWait, wns::ldk::EmptyCommand>,
        public wns::ldk::Delayed<ConstantWait>,
        public wns::events::CanTimeout
    {
    public:

        ConstantWait(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        virtual
        ~ConstantWait();

        virtual void onFUNCreated();

        /// Delayed interface realization
        void
        processIncoming(const wns::ldk::CompoundPtr& compound);
        void
        processOutgoing(const wns::ldk::CompoundPtr& compound);
        bool
        hasCapacity() const;
        const wns::ldk::CompoundPtr
        hasSomethingToSend() const;
        wns::ldk::CompoundPtr
        getSomethingToSend();

        /** @brief CanTimeout interface realisation */
        virtual void
        onTimeout();

    private:
        wns::ldk::CompoundPtr currentFrame;

        const std::string managerName;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;

        /** @brief Duration to delay frames */
        const wns::simulator::Time sifsDuration;

        wns::logger::Logger logger;
    };

} // timing
} // lowerMAC
} // wifimac

#endif // WIFIMAC_LOWERMAC_TIMING_CONSTANTWAIT_HPP
