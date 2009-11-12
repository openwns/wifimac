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

#ifndef WIFIMAC_DRAFTN_BLOCKACK_HPP
#define WIFIMAC_DRAFTN_BLOCKACK_HPP

#include <WIFIMAC/draftn/BlockACKCommand.hpp>
#include <WIFIMAC/draftn/TransmissionQueue.hpp>
#include <WIFIMAC/draftn/ReceptionQueue.hpp>

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

            typedef wns::container::Registry<wns::service::dll::UnicastAddress, BlockACKCommand::SequenceNumber> AddrSNMap;

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
            void
            registerObserver(IBlockACKObserver *o) 
                {
                    observers.push_back(o);
                }

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

            virtual bool
            doIsAccepting(const wns::ldk::CompoundPtr& compound) const;

            const size_t
            getMaxOnAir()
                {
                    return maxOnAir;
                }

            wifimac::management::PERInformationBase*
            getPERMIB()
                {
                    return this->perMIB;
                }

    protected:
            /// Logger
            wns::logger::Logger logger;

            /// Storage of outgoing, non-ack'ed frames
            TransmissionQueue *currentTxQueue;

            /** @brief Compute storage size (txQueue + sum(rxQueues) */
            virtual unsigned int
            storageSize() const;

            /// Maximum number of stored bits (rx+tx+air)
            const Bit capacity;

           /// calculation of size (e.g. by bits or pdus)
            std::auto_ptr<wns::ldk::buffer::SizeCalculator> sizeCalculator;

            /// Window size of a link
            const size_t maxOnAir;

            /**
             * @brief BlockACK state can be
             *   * idle: No ongoing transmission
             *   * waitForACK: Compound is send, waiting for beginning of
             *        reception of ACK
             *   * receiving: Reception has started, not sure if it is the
             *        (Block)ACK
             *   * receptionFinished: Got RxEndIndication and waiting for PHY to
             *        deliver the frame
             */
            enum BAState {
                idle,
                waitForACK,
                receiving,
                receptionFinished
            } baState;

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
            void
            processIncomingACKSNs(std::set<BlockACKCommand::SequenceNumber> ackSNs);

    private:

            bool
            isAccepting(const wns::ldk::CompoundPtr& compound) const;

            /** @brief Debug helper function */
            void printTxQueueStatus() const;

            const std::string managerName;
            const std::string rxStartEndName;
            const std::string txStartEndName;
            const std::string sendBufferName;
            const std::string perMIBServiceName;

            const wifimac::convergence::PhyMode blockACKPhyMode;

            /// Duration of the Short InterFrame Space
            const wns::simulator::Time sifsDuration;
            /// Maximum expected duration of BlockACK
            const wns::simulator::Time maximumACKDuration;
            /// Duration between start of preamble and rx indication
            const wns::simulator::Time ackTimeout;

            /// size of BlockACK
            const Bit baBits;
            /// size of BlockACKReques
            const Bit baReqBits;

            /// maximum number of transmissions before drop
            const size_t maximumTransmissions;

            /// transmit as BAreq as early as possible without reaching maxOnAir
            const bool impatientBAreqTransmission;

            /// Storage of incoming, non-ordered frames
            wns::container::Registry<wns::service::dll::UnicastAddress, ReceptionQueue*> rxQueues;

            /// indication that ACK must be transmitted
            wns::service::dll::UnicastAddress hasACKfor;

            /// mapping from unicast addresses to compound transmission SNs for
            /// each link (receiver), the first SN of the next transmission
            /// round is stored
            AddrSNMap nextTransmissionSN;

            struct Friends
            {
                /// Pointer to the transmission buffer to check if frames are pending
                wns::ldk::DelayedInterface* sendBuffer;

                wifimac::lowerMAC::Manager* manager;
            } friends;

            /// Signalling successfull/failed transmission attempts to the
            /// management information base
            wifimac::management::PERInformationBase* perMIB;

            /// probing the number of tx attempts until successfull transmission
            /// or retry abort
            wns::probe::bus::ContextCollectorPtr numTxAttemptsProbe;

            std::vector<IBlockACKObserver *> observers;
        };

} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_BLOCKACK_HPP
