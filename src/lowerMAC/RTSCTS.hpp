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

#ifndef WIFIMAC_LOWERMAC_RTSCTS_HPP
#define WIFIMAC_LOWERMAC_RTSCTS_HPP

#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/lowerMAC/ITransmissionCounter.hpp>

#include <WIFIMAC/convergence/INetworkAllocationVector.hpp>
#include <WIFIMAC/convergence/IRxStartEnd.hpp>
#include <WIFIMAC/convergence/ITxStartEnd.hpp>
#include <WIFIMAC/management/ProtocolCalculator.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>

#include <WNS/simulator/Time.hpp>
#include <WNS/events/CanTimeout.hpp>

#include <WNS/Observer.hpp>

namespace wifimac { namespace lowerMAC {

    class RTSCTSCommand:
        public wns::ldk::Command
    {
    public:
        struct {} local;
        struct {
            bool isRTS;
        } peer;
        struct {} magic;
    };


   class RTSCTS:
        public wns::ldk::fu::Plain<RTSCTS, RTSCTSCommand>,
        public wns::ldk::Delayed<RTSCTS>,
        public wns::events::CanTimeout,
        public wns::Observer<wifimac::convergence::INetworkAllocationVector>,
        public wns::Observer<wifimac::convergence::IRxStartEnd>,
        public wns::Observer<wifimac::convergence::ITxStartEnd>
    {
    public:

        RTSCTS(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        virtual
        ~RTSCTS();

        // observer RxStartEnd
        void
        onRxStart(wns::simulator::Time expRxTime);
        void
        onRxEnd();
        void
        onRxError();

        // observer TxStartEnd
        void
        onTxStart(const wns::ldk::CompoundPtr& compound);
        void
        onTxEnd(const wns::ldk::CompoundPtr& compound);

        // observer NAV
        void
        onNAVBusy(const wns::service::dll::UnicastAddress setter);
        void
        onNAVIdle();

        /**
		 * @brief SDU and PCI size calculation
		 */
        void calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const;

    private:
        /** @brief Delayed Interface */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);
        bool hasCapacity() const;
        const wns::ldk::CompoundPtr hasSomethingToSend() const;
        wns::ldk::CompoundPtr getSomethingToSend();

        void onFUNCreated();

        /** @brief CanTimeout interface */
        void onTimeout();

        wns::ldk::CompoundPtr
        prepareRTS(const wns::ldk::CompoundPtr& mpdu);

        wns::ldk::CompoundPtr
        prepareCTS(const wns::ldk::CompoundPtr& rts);

        const std::string phyUserName;
        const std::string managerName;
        const std::string arqName;
        const std::string navName;
        const std::string rxsName;
        const std::string txStartEndName;
        const std::string protocolCalculatorName;

        /** @brief Duration of the Short InterFrame Space */
        const wns::simulator::Time sifsDuration;

        const wns::simulator::Time expectedACKDuration;
        const wns::simulator::Time expectedCTSDuration;
        const wns::simulator::Time preambleProcessingDelay;

        const int phyModeId;

        const Bit rtsBits;
        const Bit ctsBits;
        const Bit rtsctsThreshold;
        const bool rtsctsOnTxopData;

        bool nav;
        wns::service::dll::UnicastAddress navSetter;

        wns::logger::Logger logger;

        wns::ldk::CompoundPtr pendingRTS;
        wns::ldk::CompoundPtr pendingCTS;
        wns::ldk::CompoundPtr pendingMPDU;

	wifimac::management::ProtocolCalculator *protocolCalculator;
        struct Friends
        {
            wifimac::convergence::PhyUser* phyUser;
            wifimac::lowerMAC::Manager* manager;
            wifimac::lowerMAC::ITransmissionCounter* arq;
        } friends;

        enum RTSCTSState
        {
            idle,
            transmitRTS,
            waitForCTS,
            receiveCTS
        } state;

        wns::simulator::Time ctsPrepared;
        wns::simulator::Time lastTimeout;
    };


} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_CONSTANTWAIT_HPP
