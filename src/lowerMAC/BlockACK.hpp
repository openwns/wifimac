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

#ifndef WIFIMAC_LOWERMAC_BlockACK_HPP
#define WIFIMAC_LOWERMAC_BlockACK_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/management/PERInformationBase.hpp>
#include <WIFIMAC/lowerMAC/ITransmissionCounter.hpp>


#include <WIFIMAC/convergence/IRxStartEnd.hpp>
#include <WIFIMAC/convergence/ITxStartEnd.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>
#include <WNS/ldk/arq/ARQ.hpp>
#include <WNS/ldk/probe/Probe.hpp>

#include <WNS/probe/bus/ContextCollector.hpp>
#include <WNS/events/CanTimeout.hpp>
#include <WNS/Observer.hpp>
#include <WNS/RoundRobin.hpp>

namespace wifimac {
    namespace lowerMAC {

        typedef enum {I, BA, BAREQ} BlockAckFrameType;

        class BlockACKCommand:
        public wns::ldk::arq::ARQCommand
        {
        public:

            struct {} local;
            struct {
                BlockAckFrameType type;

                // sequence number of data frame
                SequenceNumber sn;

                // sequence numbers of ACK frame
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
        typedef std::pair<wns::ldk::CompoundPtr, Bit> CompoundPtrWithSize;

        class TransmissionQueue
        {
        public:
            TransmissionQueue(BlockACK* parent_, size_t maxOnAir_, wns::service::dll::UnicastAddress adr_);
            ~TransmissionQueue();

            void processOutgoing(const wns::ldk::CompoundPtr& compound);
            const wns::ldk::CompoundPtr hasData() const;
            wns::ldk::CompoundPtr getData();
            void processIncomingACK(std::set<BlockACKCommand::SequenceNumber> ackSNs);
            void missingACK();
            const size_t onAirSize() const
                { return onAirQueue.size(); }
            const size_t waitingSize() const
                { return txQueue.size(); }
            const Bit sizeInBit() const;
            const bool waitsForACK() const;

        private:
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

        class ReceptionQueue
        {
        public:
            ReceptionQueue(BlockACK* parent_, BlockACKCommand::SequenceNumber firstSN_, wns::service::dll::UnicastAddress adr_ );
            ~ReceptionQueue();

            void processIncomingData(const wns::ldk::CompoundPtr& compound);
            void processIncomingACKreq(const wns::ldk::CompoundPtr& compound);
            const wns::ldk::CompoundPtr hasACK() const;
            wns::ldk::CompoundPtr getACK();
            const size_t size() const;
            const Bit sizeInBit() const;

        private:
            void purgeRxStorage();

            const BlockACK* parent;
            const wns::service::dll::UnicastAddress adr;
            BlockACKCommand::SequenceNumber waitingForSN;
            std::map<BlockACKCommand::SequenceNumber, CompoundPtrWithSize> rxStorage;
            std::set<BlockACKCommand::SequenceNumber> rxSNs;
            wns::ldk::CompoundPtr blockACK;
        };

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

            BlockACK(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

            virtual ~BlockACK();

            /// Initialization
            void onFUNCreated();

            /// Delayed interface realization
            void processIncoming(const wns::ldk::CompoundPtr& compound);
            void processOutgoing(const wns::ldk::CompoundPtr& compound);
            bool hasCapacity() const;

            /// ARQ interface realization
            virtual const wns::ldk::CompoundPtr hasACK() const;
            virtual const wns::ldk::CompoundPtr hasData() const;
            virtual wns::ldk::CompoundPtr getACK();
            virtual wns::ldk::CompoundPtr getData();

            /// CanTimeout interface realization
            void onTimeout();

            /// observer RxStartEnd
            void onRxStart(wns::simulator::Time expRxTime);
            void onRxEnd();
            void onRxError();

            /// observer TxStartEnd
            void onTxStart(const wns::ldk::CompoundPtr& compound);
            void onTxEnd(const wns::ldk::CompoundPtr& compound);

            /// Interface to signal unexpected transmission failures
            void
            transmissionHasFailed(const wns::ldk::CompoundPtr& compound);
            size_t
            getTransmissionCounter(const wns::ldk::CompoundPtr& compound) const;
            void
            copyTransmissionCounter(const wns::ldk::CompoundPtr& src, const wns::ldk::CompoundPtr& dst);

            /// SDU and PCI size calculation
            void calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const;

    private:
            void printTxQueueStatus() const;
            Bit sizeInBit() const;

            const std::string managerName;
            const std::string rxStartEndName;
            const std::string txStartEndName;
            const std::string perMIBServiceName;

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

            /**
             * @brief Round-robin accessible storage of receiving link peers
             *
             * The blockACK has one transmission queue for each link. For a fair
             * activation of each link, a first approach is a non-preemtive
             * round-robin strategy:
             * Each transmission queue is allowed to transmit its packets in a row
             * until (a) no more waiting packets exist or (b) it concludes the
             * sending by a BlockACKrequest.
             * This is ensured by (1) remembering the current receiver and (2) by
             * pausing the rr iteration until BAreq is send or the waitingSize is zero
             **/
            wns::RoundRobin<wns::service::dll::UnicastAddress> receivers;
            /// Remember current receiver
            mutable wns::service::dll::UnicastAddress currentReceiver;
            /// Stop receiver round robin in hasData() until data is send
            mutable bool pauseRoundRobin;
            /// Storage of outgoing, non-ack'ed frames
            wns::container::Registry<wns::service::dll::UnicastAddress, TransmissionQueue*> txQueues;
            /// Storage of incoming, non-ordered frames
            wns::container::Registry<wns::service::dll::UnicastAddress, ReceptionQueue*> rxQueues;

            /// indication that ACK must be transmitted
            wns::service::dll::UnicastAddress hasACKfor;

            /// Store the address of the last receiver, required for the onTimeout
            wns::service::dll::UnicastAddress lastTransmissionTo;

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
        };

} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_BLOCKACK_HPP
