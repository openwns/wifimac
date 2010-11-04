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

#ifndef WIFIMAC_DRAFTN_RTSCTSWITHFLA_HPP
#define WIFIMAC_DRAFTN_RTSCTSWITHFLA_HPP

#include <WIFIMAC/lowerMAC/RTSCTS.hpp>

#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/lowerMAC/RateAdaptation.hpp>
#include <WIFIMAC/lowerMAC/ITransmissionCounter.hpp>
#include <WIFIMAC/lowerMAC/RTSCTS.hpp>

#include <WIFIMAC/convergence/INetworkAllocationVector.hpp>
#include <WIFIMAC/convergence/IRxStartEnd.hpp>
#include <WIFIMAC/convergence/ITxStartEnd.hpp>
#include <WIFIMAC/management/ProtocolCalculator.hpp>
#include <WIFIMAC/draftn/SINRwithMIMOInformationBase.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>

#include <WNS/ldk/probe/Probe.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>

#include <WNS/simulator/Time.hpp>
#include <WNS/events/CanTimeout.hpp>

#include <WNS/Observer.hpp>

namespace wifimac { namespace draftn {

    class RTSCTSwithFLACommand:
        public wns::ldk::Command,
        public wifimac::lowerMAC::RTSProviderCommand
//        public wifimac::lowerMAC::RTSCTSCommand
    {
    public:
        struct {} local;

        struct {
            bool isFLARequest;
            bool isFLAResponse;
            wns::Ratio cqi;
            std::vector< std::vector<wns::Ratio> > mimoFactors;
            bool isRTS;
        } peer;

        struct {
            Bit frameSize;
        } magic;

        bool isRTS() const
            {
                return(this->peer.isRTS);
            }
    };


   /**
    * @brief Combination of Fast Link Adaptation and RTS/CTS frame exchange
    */
   class RTSCTSwithFLA:
        public wns::ldk::fu::Plain<RTSCTSwithFLA, RTSCTSwithFLACommand>,
        public wns::ldk::Delayed<RTSCTSwithFLA>,
        public wns::events::CanTimeout,
        public wns::Observer<wifimac::convergence::INetworkAllocationVector>,
        public wns::Observer<wifimac::convergence::IRxStartEnd>,
        public wns::Observer<wifimac::convergence::ITxStartEnd>,
        public wns::ldk::probe::Probe
    {
    public:

        RTSCTSwithFLA(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        virtual
        ~RTSCTSwithFLA();

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
        const std::string raName;
        const std::string managerName;
        const std::string arqName;
        const std::string navName;
        const std::string rxsName;
        const std::string txStartEndName;
        const std::string protocolCalculatorName;
        const std::string sinrMIBServiceName;

        const wns::simulator::Time estimatedValidity;

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

        wifimac::draftn::SINRwithMIMOInformationBase* sinrMIB;
        wifimac::management::ProtocolCalculator *protocolCalculator;

        struct Friends
        {
            wifimac::convergence::PhyUser* phyUser;
            wifimac::lowerMAC::Manager* manager;
            wifimac::lowerMAC::RateAdaptation *ra;
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
