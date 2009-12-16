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

#ifndef WIFIMAC_DRAFTN_BLOCKACKCOMMAND_HPP
#define WIFIMAC_DRAFTN_BLOCKACKCOMMAND_HPP

#include <WNS/ldk/arq/ARQ.hpp>
#include <set>

namespace wifimac {
    namespace draftn {

        typedef enum {I, BA, BAREQ} BlockAckFrameType;

        /**
         * @brief The Block ACK command implements (a) the SN for regular MSDUs,
         * (b) the Block ACK Request and (c) the Block ACK reply.
         *
         * The combined implementation is neccessary because the BlockACK FU can
         * add only one type of command to the outgoing FUs. Hence, this command
         * combines all three BlockACK-relevant commands:
         * -# The sequence number of each MSDU, which is needed to identify
         *    missing MSDUs on the peer side.
         * -# The start sequence number contained in a BlockACK request
         * -# The set of sequence numbers contained (in a coded form) in the
         *    BlockACK reply.
         * The three different commands are separated by the BlockAckFrameType,
         * which can be I, BA or BAREQ.
         */
        class BlockACKCommand:
            public wns::ldk::arq::ARQCommand
        {
        public:

            struct {} local;
            struct {
                /** @brief (sub-) type of this command */
                BlockAckFrameType type;

                /**
                 * @brief Sequence number of the MSDU or starting SN in the
                 *   blockACK Request / Reply
                 */
                SequenceNumber sn;

                /**
                 * @brief Set of sequence numbers in the BlockACK reply
                 */
                std::set<SequenceNumber> ackSNs;
            } peer;
            struct {} magic;

            BlockACKCommand()
                {
                    peer.type = I;
                    peer.sn = 0;
                    peer.ackSNs.clear();
                }

            virtual bool
            isACK() const
                {
                    return peer.type == BA;
                }

            virtual bool
            isACKreq() const
                {
                    return peer.type == BAREQ;
                }
        };

} // draftn
} // wifimac

#endif // WIFIMAC_LOWERMAC_BLOCKACKCOMMAND_HPP
