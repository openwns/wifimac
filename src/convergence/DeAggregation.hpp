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

#ifndef WIFIMAC_CONVERGENCE_DEAGGREGATION_HPP
#define WIFIMAC_CONVERGENCE_DEAGGREGATION_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/TxDurationSetter.hpp>
#include <WIFIMAC/convergence/ITxStartEnd.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>

#include <WNS/events/CanTimeout.hpp>
#include <WNS/Observer.hpp>

namespace wifimac { namespace convergence {

    class DeAggregationCommand:
		public wns::ldk::Command,
        public wifimac::convergence::TxDurationProviderCommand
	{
    public:
        DeAggregationCommand()
        {
            local.txDuration = 0;
            peer.finalFragment = true;
            peer.singleFragment = true;
        };

        struct {
            wns::simulator::Time txDuration;
        } local;

        struct {
            bool finalFragment;
            bool singleFragment;
        } peer;

        struct {} magic;

        wns::simulator::Time getDuration() const { return local.txDuration; }
	};


	class DeAggregation :
		public wns::ldk::fu::Plain<DeAggregation, DeAggregationCommand>,
        public wns::ldk::Delayed<DeAggregation>,
        public wns::events::CanTimeout,
        public TxStartEndNotification,
        public wns::Observer<wifimac::convergence::ITxStartEnd>
	{
	public:

		DeAggregation(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

		virtual
		~DeAggregation();

		/**
		 * @brief SDU and PCI size calculation
		 */
		void calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const;

        /** @brief Observe TxStartEnd of single fragments */
        void
        onTxStart(const wns::ldk::CompoundPtr& compound);
        void
        onTxEnd(const wns::ldk::CompoundPtr& compound);

	private:
		/** @brief Delayed Interface realization */
		void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);
        bool hasCapacity() const;
        const wns::ldk::CompoundPtr hasSomethingToSend() const;
        wns::ldk::CompoundPtr getSomethingToSend();

		void onFUNCreated();

        /** @brief CanTimeout realization */
        void onTimeout();

        const std::string managerName;
        const std::string phyUserName;
        const std::string txStartEndName;
        const std::string aggregationCommandName;

        /** @brief Storage of outgoing fragments of a single aggregated compound */
        std::deque<wns::ldk::CompoundPtr> txQueue;
        /** @brief Storage of aggregated compounds until all fragments are send */
        wns::ldk::CompoundPtr currentTxCompound;
        /** @brief Storage of received fragments until last fragment is received
         * (or timeout)
         */
        wns::ldk::CompoundPtr currentRxContainer;
        /** @brief signal the tx start only for the first fragment */
        bool doSignalTxStart;

        /** @brief Number of entries in current rx container */
        int numEntries;

        wns::logger::Logger logger;

		struct Friends
		{
			wifimac::convergence::PhyUser* phyUser;
            wifimac::lowerMAC::Manager* manager;
		} friends;
	};


} // mac
} // wifimac

#endif // WIFIMAC_SCHEDULER_CONSTANTWAIT_HPP
