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

#ifndef WIFIMAC_LOWERMAC_TXOP_HPP
#define WIFIMAC_LOWERMAC_TXOP_HPP

#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/lowerMAC/NextFrameGetter.hpp>
#include <WIFIMAC/lowerMAC/RateAdaptation.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>
#include <WNS/ldk/Delayed.hpp>

#include <WNS/events/CanTimeout.hpp>

#include <WNS/Observer.hpp>

namespace wifimac { namespace lowerMAC {

    class TXOP:
        public wns::ldk::fu::Plain<TXOP, wns::ldk::EmptyCommand>,
        public wns::ldk::Processor<TXOP>
    {
    public:

        TXOP(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        virtual
        ~TXOP();

        // observer TxStartEnd
        void
        onTxStart(const wns::ldk::CompoundPtr& compound);
        void
        onTxEnd(const wns::ldk::CompoundPtr& compound);

    private:
        /** @brief Processor Interface Implementation */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

        bool doIsAccepting(const wns::ldk::CompoundPtr& compound) const;

        void onFUNCreated();

        const std::string managerName;
        const std::string phyUserName;
        const std::string nextFrameHolderName;
        const std::string raName;

        /** @brief Duration of the Short InterFrame Space */
        const wns::simulator::Time sifsDuration;
        const wns::simulator::Time expectedACKDuration;
        const wns::simulator::Time txopLimit;
        const bool singleReceiver;

        wns::simulator::Time remainingTXOPDuration;
        wns::service::dll::UnicastAddress txopReceiver;

        wns::logger::Logger logger;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
            wifimac::convergence::PhyUser* phyUser;
            wns::ldk::DelayedInterface* nextFrameHolder;
            wifimac::lowerMAC::RateAdaptation* ra;
        } friends;

    };


} // lowerMAC
} // wifimac

#endif // WIFIMAC_LOWERMAC_TXOP_HPP
