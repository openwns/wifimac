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

#ifndef WIFIMAC_DRAFTN_BlockACK_HPP
#define WIFIMAC_DRAFTN_BlockACK_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/management/PERInformationBase.hpp>
#include <WIFIMAC/lowerMAC/ITransmissionCounter.hpp>

#include <WIFIMAC/convergence/IRxStartEnd.hpp>
#include <WIFIMAC/convergence/ITxStartEnd.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>
#include <WNS/ldk/arq/ARQ.hpp>
#include <WNS/ldk/probe/Probe.hpp>
#include <WNS/ldk/buffer/Buffer.hpp>

#include <WNS/probe/bus/ContextCollector.hpp>
#include <WNS/events/CanTimeout.hpp>
#include <WNS/Observer.hpp>
#include <WNS/RoundRobin.hpp>

#include <WIFIMAC/draftn/IBlockACKObserver.hpp>

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

        class BlockACK;


        /**
         * @brief Store each compound together with its computed size
         */
        typedef std::pair<wns::ldk::CompoundPtr, unsigned int> CompoundPtrWithSize;

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
        public:
            TransmissionQueue(BlockACK* parent_,
                              size_t maxOnAir_,
                              wns::service::dll::UnicastAddress adr_,
                              BlockACKCommand::SequenceNumber sn_,
                              wifimac::management::PERInformationBase* perMIB_);
            ~TransmissionQueue();

            /** @brief Append outgoing compound to the txQueue */
            void
            processOutgoing(const wns::ldk::CompoundPtr& compound,
                            const unsigned int size);

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
                { return nextSN; }

        private:
            bool
            isSortedBySN(const std::deque<CompoundPtrWithSize> q) const;

            wifimac::management::PERInformationBase* perMIB;
            const BlockACK* parent;
            const size_t maxOnAir;
            const wns::service::dll::UnicastAddress adr;
            std::deque<CompoundPtrWithSize> txQueue;
            std::deque<CompoundPtrWithSize> onAirQueue;
            BlockACKCommand::SequenceNumber nextSN;
            wns::ldk::CompoundPtr baREQ;
            bool waitForACK;
            bool baReqRequired;
        };

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

        typedef wns::container::Registry<wns::service::dll::UnicastAddress, BlockACKCommand::SequenceNumber> AddrSNMap;

        /**
         * @brief Block ACK according to IEEE 802.11n Draft 8.0 / IEEE
         * 802.11-2007
         *
         * This FU implements the block acknowledgement with direct BlockACK as
         * described int IEEE 802.11-2007 and refined in IEEE 802.11n Draft
         * 8.0. The Block ACK message sequence chart for an error-free channel
         * is the following (assuming maxOnAir is given in number of frames):
         * \msc
         *   Sender, Receiver;
         *
         *   Sender->Receiver [ label = "RTS" ];
         *   Receiver->Sender [ label = "CTS" ];
         *   Sender->Receiver [ label = "Frame SN i"];
         *   Sender->Receiver [ label = "Frame SN i+1"];
         *   Sender->Receiver [ label = "Frame SN i+2"];
         *   ...;
         *   Sender->Receiver [ label = "Frame SN i+maxOnAir"];
         *   Sender->Receiver [ label = "Block ACK Request (SN = i)"];
         *   Receiver->Sender [ label = "Block ACK Reply (i, i+1, ..., i+maxOnAir)"];
         * \endmsc
         *
         * Thus, the block ack decreases the overhead of IEEE 802.11 by reducing
         * the multiple neccessary ACKs for each frame to one block ack request
         * / reply for each maxOnAir frames. Furthermore, the BlockACK can be
         * easily combined with a frame aggregation FU to further improve the
         * efficiency, reducing the number of backoff attempts.
         *
         * The BlockACK FU implementation follows the design of ARQs in the ldk
         * by deriving from wns::ldk::arq::ARQ, but enhances it with the
         * neccessary functionality for IEEE 802.11-style (i.e. immediate) ARQ,
         * based on wifimac::convergence::ITxStartEnd and
         * wifimac::convergence::IRxStartEnd. This is similar to the much
         * simpler wifimac::lowerMAC::StopAndWaitARQ.
         *
         * To handle outgoing and incoming compounds, the BlockACK uses the two
         * classes wifimac::draftN::TransmissionQueue  and
         * wifimac::draftN::ReceptionQueue:
         * - A BlockACK FU has at most one current receiver peer node and for
         *   this node one instance of wifimac::draftN::TransmissionQueue. This
         *   queue contains all compounds which are intended for this receiver,
         *   both not yet transmitted and on the air.
         * - A BlockACK FU has multiple instances of
         *   wifimac::draftN::ReceptionQueue, one for each transmitter peer
         *   node. They are used to store out-of-order compounds and to generate
         *   the BlockACK reply.
         *
         * A BlockACK FU can be set to patient or impatient:
         *
         * - If set to patient, it waits as long as possible with the
         *   transmission of the BlockACK request, i.e. until the limit of
         *   frames in the air is reached or a frame to a different receiving
         *   peer node is due.
         * - If set to impatient, the BlockACK request is transmitted if no
         *   frames to the current receiver are pending in the upper FU.
         */
        class BlockACK:
            public wns::ldk::arq::ARQ,
            public wns::ldk::fu::Plain<BlockACK, BlockACKCommand>,
            public wns::ldk::Delayed<BlockACK>,
            public wns::events::CanTimeout,
            public wns::Observer<wifimac::convergence::IRxStartEnd>,
            public wns::Observer<wifimac::convergence::ITxStartEnd>,
            public wifimac::lowerMAC::ITransmissionCounter,
            public wns::ldk::probe::Probe
        {
            friend class TransmissionQueue;
            friend class ReceptionQueue;

        public:

            /** @brief Regular FU Constructor */
            BlockACK(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

            /** @brief Copy Constructor, required when std::auto_ptr is used */
            BlockACK(const BlockACK& other);

            /** @brief Destructor */
            virtual ~BlockACK();

            /// Initialization
            void
            onFUNCreated();

	    /// observers get called after an (un)successful transmission
	    void registerObserver(IBlockACKObserver *o) {observers.push_back(o);}

            /**
             * @brief Processing of incoming (received) compounds
             *
             * Depending on the compound type and its transmitter, the correct
             * function is applied:
             * - processIncomingACKSNs in case of a received ACK
             * - processIncomingACKreq of the matching rxQueue in case of ACKreq
             * - processIncomingData of the matching rxQueue otherwise
             */
            void
            processIncoming(const wns::ldk::CompoundPtr& compound);

            /**
             * @brief Processing of outgoing (to be transmitted) compounds
             *
             * The BlockACK FU owns only one transmission queue which is
             * operated on until all compounds are either successfully
             * transmitted or discarded. Hence, if a compound for a different
             * receiving peer node arrives, it is temporary stored until the
             * current txQueue has finished operation.
             *
             * As speciallity, the function tries to get as much frames as
             * possible from the upper FU before starting the transmission using
             * a getReceptor()->wakeup() recursion.
             */
            void
            processOutgoing(const wns::ldk::CompoundPtr& compound);

            /**
             * @brief Sets the conditions to accept a new compound to transmit
             *
             * These conditions are
             * #- The storage capacity of the FU (size txQueue + sizes rxQueues)
             *    is not exhausted.
             * #- The last compound accepted has the same receiver as the
             *    current transmission queue
             * #- The transmission queue does not wait for an ACK, i.e. there
             *    are no frames on the air.
             * #- The number of frames in the txQueue is below the number which
             *    can be send in the next round.
             */
            bool
            hasCapacity() const;

            /**
             * @brief Queries the rxQueue for which a BlockACK request has been
             * received for the BlockACK reply.
             */
            virtual const wns::ldk::CompoundPtr
            hasACK() const;

            /**
             * @brief Checks if the current txQueue has pending compounds
             */
            virtual const wns::ldk::CompoundPtr
            hasData() const;

            /**
             * @brief Returns the pending BlockACK reply
             */
            virtual wns::ldk::CompoundPtr
            getACK();

            /**
             * @brief Returns the currently pending compound
             */
            virtual wns::ldk::CompoundPtr
            getData();

            /**
             * @brief CanTimeout interface realization, required for missing
             * ACKs.
             */
            void onTimeout();

            /** @brief Observe rxStart to stop the timeout for ACK reception */
            void onRxStart(wns::simulator::Time expRxTime);

            /** @brief Observe rxEnd to signal the upcoming ACK reception */
            void onRxEnd();

            /** @brief Does nothing, required by IRxStartEnd interface */
            void onRxError();

            /** @brief Signals start of transmission */
            void onTxStart(const wns::ldk::CompoundPtr& compound);

            /** @brief Signals end of own transmission -> set timeout for ACK*/
            void onTxEnd(const wns::ldk::CompoundPtr& compound);

            /** 
             * @brief Signal from RTS/CTS that the transmission has failed
             *
             * A failed CTS is handled in the same way as a missing ACK or a
             * BlockACK reply with an empty SN-set
             */
            void
            onTransmissionHasFailed(const wns::ldk::CompoundPtr& compound);

            /** @brief Read the tx counter */
            unsigned int
            getTransmissionCounter(const wns::ldk::CompoundPtr& compound) const;

            /** @brief Copy the tx counter to anther FU. */
            void
            copyTransmissionCounter(const wns::ldk::CompoundPtr& src, const wns::ldk::CompoundPtr& dst);

            /// SDU and PCI size calculation
            void calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const;

            /** @brief Returns the pointer to the wifimac::lowerMAC::Manager FU */
            wifimac::lowerMAC::Manager*
            getManager() const
                {
                    return friends.manager;
                }

    protected:
	    virtual bool
	    doIsAccepting(const wns::ldk::CompoundPtr& compound) const;

    private:
            /**
             * @brief Manages an incoming ACK (or rather the set of SNs)
             *
             * After the forwarding of the set of SNs to the current txQueue
             * with processIncomingACK(), the function checks if the txQueue has
             * finished processing the current row of frames. If yes, it is
             * deleted and the current receiver is reset, so that a new round of
             * transmissions can start (possible with the one compound for a
             * different receiver that has been stored temporarily)
             */
            void processIncomingACKSNs(std::set<BlockACKCommand::SequenceNumber> ackSNs);
	    bool isAccepting(const wns::ldk::CompoundPtr& compound) const;

            /** @brief Debug helper function */
            void printTxQueueStatus() const;

            /** @brief Compute storage size (txQueue + sum(rxQueues) */
            unsigned int storageSize() const;

            const std::string managerName;
            const std::string rxStartEndName;
            const std::string txStartEndName;
            const std::string sendBufferName;
            const std::string perMIBServiceName;

            const wifimac::convergence::PhyMode blockACKPhyMode;

            /// Pointer to the transmission buffer to check if frames are pending
            wns::ldk::DelayedInterface *sendBuffer;
            /// Duration of the Short InterFrame Space
            const wns::simulator::Time sifsDuration;
            /// Expected duration of BlockACK
            const wns::simulator::Time expectedACKDuration;
            /// Duration between start of preamble and rx indication
            const wns::simulator::Time preambleProcessingDelay;
            /// Maximum number of stored bits (rx+tx+air)
            const Bit capacity;
            /// Window size of a link
            const size_t maxOnAir;

            /// size of BlockACK
            const Bit baBits;
            /// size of BlockACKReques
            const Bit baReqBits;

            /// maximum number of transmissions before drop
            const size_t maximumTransmissions;

            /// transmit as BAreq as early as possible without reaching maxOnAir
            const bool impatientBAreqTransmission;

            /// current transmission receiver
            mutable wns::service::dll::UnicastAddress currentReceiver;

            /// Storage of outgoing, non-ack'ed frames
            TransmissionQueue *txQueue;

            /// Storage of incoming, non-ordered frames
            wns::container::Registry<wns::service::dll::UnicastAddress, ReceptionQueue*> rxQueues;

            /// indication that ACK must be transmitted
            wns::service::dll::UnicastAddress hasACKfor;

            /// mapping from unicast addresses to compound transmission SNs for
            /// each link (receiver), the first SN of the next transmission
            /// round is stored
            AddrSNMap nextTransmissionSN;

            /**
             * @brief BlockACK state can be
             *   * idle: No ongoing transmission
             *   * waitForACK: Compound is send, waiting for beginning of
             *        reception of ACK
             *   * receiving: Reception has started, not sure if it is the
             *        (Block)ACK
             */
            enum BAState {
                idle,
                waitForACK,
                receiving
            } baState;

            /// Logger
            wns::logger::Logger logger;

            struct Friends
            {
                wifimac::lowerMAC::Manager* manager;
            } friends;

            /// Signalling successfull/failed transmission attempts to the
            /// management information base
            wifimac::management::PERInformationBase* perMIB;

            /// probing the number of tx attempts until successfull transmission
            /// or retry abort
            wns::probe::bus::ContextCollectorPtr numTxAttemptsProbe;

            /// calculation of size (e.g. by bits or pdus)
            std::auto_ptr<wns::ldk::buffer::SizeCalculator> sizeCalculator;

	    std::vector<IBlockACKObserver *> observers;
        };

} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_BLOCKACK_HPP
