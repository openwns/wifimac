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

#ifndef WIFIMAC_LOWERMAC_STOPANDWAITARQ_HPP
#define WIFIMAC_LOWERMAC_STOPANDWAITARQ_HPP

#include <WNS/ldk/arq/StopAndWait.hpp>
#include <WIFIMAC/lowerMAC/ITransmissionCounter.hpp>

#include <WIFIMAC/convergence/IRxStartEnd.hpp>
#include <WIFIMAC/convergence/ITxStartEnd.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/management/PERInformationBase.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>

#include <WNS/Observer.hpp>
#include <WNS/ldk/Layer.hpp>
#include <WNS/simulator/Time.hpp>

#include <WNS/ldk/probe/Probe.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>

namespace wifimac { namespace lowerMAC {

    /**
     * @brief Specialization fo the wns::ldk::arq::StopAndWait for the IEEE 802.11 DCF
     */
     class StopAndWaitARQ :
        public wns::ldk::arq::StopAndWait,
        public wns::Observer<wifimac::convergence::IRxStartEnd>,
        public wns::Observer<wifimac::convergence::ITxStartEnd>,
        public wifimac::lowerMAC::ITransmissionCounter,
        public wns::ldk::probe::Probe
    {
    public:
        StopAndWaitARQ(wns::ldk::fun::FUN* fuNet, const wns::pyconfig::View& config);

        // observer ChannelState
        void
        onChannelBusy();
        void
        onChannelIdle();

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

        // implementation of ITransmissionCounter
        void
        onTransmissionHasFailed(const wns::ldk::CompoundPtr& compound);

        unsigned int
        getTransmissionCounter(const wns::ldk::CompoundPtr& compound) const;

        void
        copyTransmissionCounter(const wns::ldk::CompoundPtr& src, const wns::ldk::CompoundPtr& dst);

    private:
        void
        onFUNCreated();

        wns::ldk::CompoundPtr getData();

        void
        transmissionHasFailed(const wns::ldk::CompoundPtr& compound);

        void onTimeout();
        void processOutgoing(const wns::ldk::CompoundPtr& compound);
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        bool hasCapacity() const;

        const std::string rxsName;
        const std::string txStartEndName;
        const std::string managerName;
        const std::string perMIBServiceName;

        const int shortRetryLimit;
        const int longRetryLimit;
        const Bit rtsctsThreshold;

        int stationShortRetryCounter;
        int stationLongRetryCounter;
        int shortRetryCounter;
        int longRetryCounter;

        const wns::simulator::Time sifsDuration;
        const wns::simulator::Time maximumACKDuration;
        const wns::simulator::Time ackTimeout;

        const wifimac::convergence::PhyMode ackPhyMode;

        const bool bianchiRetryCounter;

        enum AckState {
            none,
            waitForACK,
            receiving,
            receptionFinished,
            sendingACK
        } ackState;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;

        wifimac::management::PERInformationBase* perMIB;

        wns::probe::bus::ContextCollectorPtr numTxAttemptsProbe;
    };

} // namespace lowerMAC
} // namespace wifimac

#endif //ifndef WIFIMAC_LOWERMAC_STOPANDWAITARQ_HPP
