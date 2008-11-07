/******************************************************************************
 * WiFiMac                                                                    *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2005-2007                                                    *
 * Lehrstuhl fuer Kommunikationsnetze (ComNets)                               *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: wns@comnets.rwth-aachen.de                                          *
 * www: http://wns.comnets.rwth-aachen.de                                     *
 ******************************************************************************/

#ifndef WIFIMAC_LOWERMAC_STOPANDWAITARQ_HPP
#define WIFIMAC_LOWERMAC_STOPANDWAITARQ_HPP

#include <WNS/ldk/arq/StopAndWait.hpp>
#include <WIFIMAC/lowerMAC/ITransmissionCounter.hpp>

#include <WIFIMAC/convergence/IRxStartEnd.hpp>
#include <WIFIMAC/convergence/ITxStartEnd.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/management/PERInformationBase.hpp>

#include <WNS/Observer.hpp>
#include <WNS/ldk/Layer.hpp>
#include <WNS/simulator/Time.hpp>

#include <WNS/ldk/probe/Probe.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>

namespace wifimac { namespace lowerMAC {

    /**
     * @brief StopAndWaitARQ specialization for the IEEE 802.11 DCF
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
        transmissionHasFailed(const wns::ldk::CompoundPtr& compound);

        size_t
        getTransmissionCounter(const wns::ldk::CompoundPtr& compound) const;

        void
        copyTransmissionCounter(const wns::ldk::CompoundPtr& src, const wns::ldk::CompoundPtr& dst);

    private:
        void
        onFUNCreated();

        wns::ldk::CompoundPtr getData();

        void onTimeout();
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        bool hasCapacity() const;

        const std::string rxsName;
        const std::string txStartEndName;
        const std::string managerName;
        const std::string perMIBServiceName;

        const int shortRetryLimit;
        const int longRetryLimit;
        const Bit rtsctsThreshold;

        const wns::simulator::Time sifsDuration;
        const wns::simulator::Time expectedACKDuration;
        const wns::simulator::Time preambleProcessingDelay;

        const int ackPhyModeId;

        enum AckState {
            none,
            waitForACK,
            receiving,
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
