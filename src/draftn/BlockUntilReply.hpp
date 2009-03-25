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

#ifndef WIFIMAC_DRAFTN_BLOCKUNTILREPLY_HPP
#define WIFIMAC_DRAFTN_BLOCKUNTILREPLY_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/convergence/IRxStartEnd.hpp>
#include <WIFIMAC/convergence/ITxStartEnd.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/events/CanTimeout.hpp>

namespace wifimac { namespace draftn {

    class BlockUntilReply:
        public wns::ldk::fu::Plain<BlockUntilReply, wns::ldk::EmptyCommand>,
        public wns::events::CanTimeout,
        public wns::Observer<wifimac::convergence::ITxStartEnd>,
        public wns::Observer<wifimac::convergence::IRxStartEnd>
    {
    public:
        BlockUntilReply(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
        virtual ~BlockUntilReply();

        /** @brief Initialization */
        void onFUNCreated();

        /** @brief CompoundHandlerInterface */
        void doSendData(const wns::ldk::CompoundPtr& compound);
        void doOnData(const wns::ldk::CompoundPtr& compound);
        bool doIsAccepting(const wns::ldk::CompoundPtr& compound) const;
        void doWakeup();

        /** @brief CanTimeout Interface */
        void onTimeout();

        /** @brief Observe TxStartEnd of single fragments */
        void onTxStart(const wns::ldk::CompoundPtr& compound);
        void onTxEnd(const wns::ldk::CompoundPtr& compound);

        /** @brief observer RxStartEnd */
        void onRxStart(wns::simulator::Time expRxTime);
        void onRxEnd();
        void onRxError();

    private:
        const wns::simulator::Time sifsDuration;
        const wns::simulator::Time preambleProcessingDelay;
        const std::string managerName;
        const std::string txStartEndName;
        const std::string rxStartEndName;

        bool blocked;
        enum TxStatus {
            waiting,
            transmitting,
            finished };
        TxStatus txStatus;
        wns::Birthmark blockedBy;

        wns::logger::Logger logger;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;
    };
} // end namespace draftn
} // end namespace wifimac

#endif // ifndef WIFIMAC_DRAFTN_BLOCKUNTILREPLY_HPP
