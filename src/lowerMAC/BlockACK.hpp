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
#include <WNS/ldk/buffer/Buffer.hpp>

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

        typedef std::pair<wns::ldk::CompoundPtr, unsigned int> CompoundPtrWithSize;

        class TransmissionQueue
        {
        public:
            TransmissionQueue(BlockACK* parent_,
                              size_t maxOnAir_,
                              wns::service::dll::UnicastAddress adr_,
                              BlockACKCommand::SequenceNumber sn_,
                              wifimac::management::PERInformationBase* perMIB_);
            ~TransmissionQueue();

            void
            processOutgoing(const wns::ldk::CompoundPtr& compound,
                            const unsigned int size);

            const wns::ldk::CompoundPtr
            hasData() const;

            wns::ldk::CompoundPtr
            getData();

            void
            processIncomingACK(std::set<BlockACKCommand::SequenceNumber> ackSNs);

            //void
            //missingACK();

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

        class ReceptionQueue
        {
        public:
            ReceptionQueue(BlockACK* parent_,
                           BlockACKCommand::SequenceNumber firstSN_,
                           wns::service::dll::UnicastAddress adr_);
            ~ReceptionQueue();

            void processIncomingData(const wns::ldk::CompoundPtr& compound,
                                     const unsigned int size);
            void processIncomingACKreq(const wns::ldk::CompoundPtr& compound);
            const wns::ldk::CompoundPtr hasACK() const;
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
            BlockACK(const BlockACK& other);

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
            onTransmissionHasFailed(const wns::ldk::CompoundPtr& compound);
            unsigned int
            getTransmissionCounter(const wns::ldk::CompoundPtr& compound) const;
            void
            copyTransmissionCounter(const wns::ldk::CompoundPtr& src, const wns::ldk::CompoundPtr& dst);

            /// SDU and PCI size calculation
            void calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const;

            wifimac::lowerMAC::Manager*
            getManager() const
                {
                    return friends.manager;
                }

    private:
            void processIncomingACKSNs(std::set<BlockACKCommand::SequenceNumber> ackSNs);
            void printTxQueueStatus() const;
            unsigned int storageSize() const;

            const std::string managerName;
            const std::string rxStartEndName;
            const std::string txStartEndName;
            const std::string sendBufferName;
            const std::string perMIBServiceName;
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

            /// temporary storage of first compound for the next transmission
            /// block with different receiver
            wns::ldk::CompoundPtr nextFirstCompound;

            // address of the receiver for the next transmission round after the
            // current one has been transmitted successfully
            wns::service::dll::UnicastAddress nextReceiver;

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

            // calculation of size (e.g. by bits or pdus)
            std::auto_ptr<wns::ldk::buffer::SizeCalculator> sizeCalculator;

            bool inWakeup;

        };

} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_BLOCKACK_HPP
