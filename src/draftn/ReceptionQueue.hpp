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

#ifndef WIFIMAC_DRAFTN_RECEPTIONQUEUE_HPP
#define WIFIMAC_DRAFTN_RECEPTIONQUEUE_HPP

#include <WIFIMAC/draftn/BlockACKCommand.hpp>

#include <WNS/ldk/Compound.hpp>
#include <WNS/service/dll/Address.hpp>

namespace wifimac {
    namespace draftn {


        class BlockACK;

        /**
         * @brief Queue that stores received compounds that cannot be delivered
         * (yet).
         *
         * The reception queue implements, for each transmitter, the handling of
         * received compounds. Compounds stored due to out-of-order delivery in
         * case of packet errors, until they can be delivered in order or until
         * the transceiver signals that the missing frames are discarded.
         *
         * Hence, the reception queue implements the receiver functions of the
         * BlockACK FU: processIncoming[Data|ACK] and [has|get]ACK
         */

        class ReceptionQueue
        {
            /**
             * @brief Store each compound together with its computed size
             */
            typedef std::pair<wns::ldk::CompoundPtr, unsigned int> CompoundPtrWithSize;

        public:
            ReceptionQueue(BlockACK* parent_,
                           BlockACKCommand::SequenceNumber firstSN_,
                           wns::service::dll::UnicastAddress adr_);
            ~ReceptionQueue();

            /**
             * @brief Processes the incoming compound and delivers it if it is
             * in-order.
             *
             * If the compound's SN matches the current waiting SN, the compound
             * is delivered. Otherwise, either the compound was already received
             * before (SN < waitingSN) or intermediate compounds are missing and
             * the compound has to be stored until they are received
             * successfully (SN > waitingSN). In both cases, the SN is added to
             * the set of received SNs for the next BlockACK reply.
             */
            void
            processIncomingData(const wns::ldk::CompoundPtr& compound,
                                const unsigned int size);

            /**
             * @brief Process the incoming BlockACK request and create the
             * BlockACK reply
             *
             * The BlockACK request contains a SN which indicates the start of
             * the SNs that need to be ack'ed. If compounds are discarded by the
             * transmitter (due to lifetime), this SN is higher than the current
             * waitingSN, and pending frames in the rxQueue can be delivered.
             *
             * Of course, as a reply the Block ACK reply is created, containing
             * a set of successfully received compound SNs.
             */
            void
            processIncomingACKreq(const wns::ldk::CompoundPtr& compound);

            /** @brief Indicates that an Block ACK reply is pending */
            const wns::ldk::CompoundPtr hasACK() const;

            /** @brief Returns the pending Block ACK */
            wns::ldk::CompoundPtr getACK();

            const size_t numPDUs() const
                { return rxStorage.size(); }
            const unsigned int storageSize() const;

        private:
            void purgeRxStorage();

            const BlockACK* parent;
            const wns::service::dll::UnicastAddress adr;
            BlockACKCommand::SequenceNumber waitingForSN;
            std::map<BlockACKCommand::SequenceNumber, CompoundPtrWithSize> rxStorage;
            std::set<BlockACKCommand::SequenceNumber> rxSNs;
            wns::ldk::CompoundPtr blockACK;
        };

} // draftn
} // wifimac

#endif // WIFIMAC_DRAFTN_RECEPTIONQUEUE_HPP
