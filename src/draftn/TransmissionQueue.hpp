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

#ifndef WIFIMAC_DRAFTN_TRANSMISSIONQUEUE_HPP
#define WIFIMAC_DRAFTN_TRANSMISSIONQUEUE_HPP

#include <WIFIMAC/draftn/BlockACKCommand.hpp>
#include <WIFIMAC/management/PERInformationBase.hpp>

#include <WNS/ldk/Compound.hpp>
#include <WNS/service/dll/Address.hpp>
#include <WNS/ldk/buffer/Buffer.hpp>

namespace wifimac {
    namespace draftn {

        class BlockACK;

        /**
         * @brief Queue for outgoing compounds
         *
         * The transmission queue implements the storage and access functions of
         * the BlockACK FU for outgoing compounds, most importantly the
         * processOutgoing, has|getData and processIncomingACK functions. A
         * transmission queue is only valid for a single receiver.
         *
         * Two queues are inside the transmission queue: One for the compounds
         * waiting to be transmitted (txQueue), and the other for compounds already
         * transmitted, but not acknowledged (onAirQueue). The onAirQueue cannot
         * exceed the maxOnAir limitation.
         */
        class TransmissionQueue
        {

            /**
             * @brief Store each compound together with its expiration time
             */
            typedef std::pair<wns::ldk::CompoundPtr, wns::simulator::Time> CompoundPtrWithTime;

        public:
            TransmissionQueue(BlockACK* parent_,
                              size_t maxOnAir_,
                              wns::simulator::Time maxDelay,
                              wns::service::dll::UnicastAddress adr_,
                              BlockACKCommand::SequenceNumber sn_,
                              wifimac::management::PERInformationBase* perMIB_,
                              std::auto_ptr<wns::ldk::buffer::SizeCalculator> *sizeCalculator);
            ~TransmissionQueue();

            /** @brief Append outgoing compound to the txQueue */
            void
            processOutgoing(const wns::ldk::CompoundPtr& compound);

            /**
             * @brief Gets next compound for transmission or NULL
             *
             * The availability of the next compound is dependent on many
             * factors:
             * - If a BlockACK request was send, but not yet replied, no frames
             * can be send (status waitForACK).
             * - If the txQueue is not empty and the next compound fits into the
             * maxOnAir limit, this one is available
             * - If a BlockACK request is pending it will be send (also
             * depending on the "impatient" setting of the BlockACK FU
             */
            const wns::ldk::CompoundPtr
            hasData() const;

            /**
             * @brief Returns the compound as selected by the hasData() function
             */
            wns::ldk::CompoundPtr
            getData();

            /**
             * @brief Iterate, using two iterators, over the onAirQueue and the
             *        SNs indicated in the ACK to identify packet losses.
             *
             * Two iterators are used to identify packet losses: One for the
             * onAirQueue and the other on for the SNs in the received
             * ACK. Frames are re-transmitted (by insertion at the head of the
             * txQueue) until their lifetime is exceeded.
             */
            void
            processIncomingACK(std::set<BlockACKCommand::SequenceNumber> ackSNs);

            const size_t getNumOnAirPDUs() const
                { return onAirQueue.size(); }

            const size_t getNumWaitingPDUs() const
                { return txQueue.size(); }

            const unsigned int
            onAirQueueSize() const;

            const unsigned int
            txQueueSize() const;

            const unsigned int
            storageSize() const;

            const bool
            waitsForACK() const;

            const BlockACKCommand::SequenceNumber
            getNextSN() const
                {
                    return nextSN;
                }

            const wns::service::dll::UnicastAddress
            getReceiver()
                {
                    return adr;
                }

            void setMaxDelay(wns::simulator::Time delay)
                {
                    maxDelay = delay;
                }

            wns::simulator::Time getOldestTimestamp()
                {
                    if (oldestTimestamp > wns::simulator::Time())
                        return oldestTimestamp + maxDelay;
                    else
                        return 0;
                }

            std::list<wns::ldk::CompoundPtr>
            getAirableCompounds();

            const size_t
            maxAirSize()
                {
                    return maxOnAir;
                }

            void setMaxOnAir(size_t maxAir) 
                {
                    maxOnAir = maxAir;
                }

            wns::ldk::CompoundPtr
            getTxFront();

        private:
            bool
            isSortedBySN(const std::deque<CompoundPtrWithTime> q) const;

            wns::ldk::CompoundPtr baREQ;
            wifimac::management::PERInformationBase* perMIB;
            wns::simulator::Time maxDelay;
            wns::simulator::Time oldestTimestamp;
            size_t maxOnAir;
            const BlockACK* parent;
            const wns::service::dll::UnicastAddress adr;
            std::deque<CompoundPtrWithTime> txQueue;
            std::deque<CompoundPtrWithTime> onAirQueue;
            BlockACKCommand::SequenceNumber nextSN;
            bool waitForACK;
            bool baReqRequired;
            std::auto_ptr<wns::ldk::buffer::SizeCalculator> *sizeCalculator;
        };

} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_BLOCKACK_TRANSMISSIONQUEUE_HPP
