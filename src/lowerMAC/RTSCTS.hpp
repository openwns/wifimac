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

#include <WNS/ldk/probe/Probe.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>

#include <WNS/simulator/Time.hpp>
#include <WNS/events/CanTimeout.hpp>

#include <WNS/Observer.hpp>

namespace wifimac { namespace lowerMAC {

    class RTSProviderCommand
    {
    public:
        virtual bool isRTS() const = 0;
        virtual ~RTSProviderCommand() {};
    };

    class RTSCTSCommand:
        public wns::ldk::Command,
        public RTSProviderCommand
    {
    public:
        struct {} local;
        struct {
            bool isRTS;
        } peer;
        struct {} magic;

        bool isRTS() const
            {
                return(this->peer.isRTS);
            }
    };


   /**
    * @brief Implementation of the RTS/CTS frame exchange in IEEE 802.11
    *
    * The RTS/CTS frame exchange works in the following way, assuming no
    * transmission errors.
    *
    * \msc
    *   Sender, Receiver;
    *
    *   Sender->Receiver [ label = "RTS" ];
    *   Receiver->Sender [ label = "CTS" ];
    *   Sender->Receiver [ label = "Data"];
    *   Sender->Receiver [ label = "ACK"];
    * \endmsc
    *
    * The three-way handshake of the RTS/CTS/DATA/ACK increases the overhead,
    * but also the probability for a successful frame exchange, as the RTS and
    * CTS set the NAV of all overhearing STAs in reach of both the transmitter
    * and the receiver.
    */
   class RTSCTS:
        public wns::ldk::fu::Plain<RTSCTS, RTSCTSCommand>,
        public wns::events::CanTimeout,
        public wns::Observer<wifimac::convergence::INetworkAllocationVector>,
        public wns::Observer<wifimac::convergence::IRxStartEnd>,
        public wns::Observer<wifimac::convergence::ITxStartEnd>,
        public wns::ldk::probe::Probe
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
        virtual
        void doWakeup();

        virtual
        void
        doOnData(const wns::ldk::CompoundPtr& compound);

        virtual bool
        doIsAccepting(const wns::ldk::CompoundPtr& compound) const;

        virtual void
        doSendData(const wns::ldk::CompoundPtr& compound);

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

        const wns::simulator::Time maximumACKDuration;
        const wns::simulator::Time maximumCTSDuration;
        const wns::simulator::Time preambleProcessingDelay;
        const wns::simulator::Time ctsTimeout;

        const wifimac::convergence::PhyMode rtsctsPhyMode;

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
            receiveCTS,
            receptionFinished
        } state;

        wns::simulator::Time ctsPrepared;
        wns::simulator::Time lastTimeout;

        wns::probe::bus::ContextCollectorPtr rtsSuccessProbe;
    };


} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_CONSTANTWAIT_HPP
