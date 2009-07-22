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

#ifndef WIFIMAC_CONVERGENCE_FRAMESYNCHRONIZATION_HPP
#define WIFIMAC_CONVERGENCE_FRAMESYNCHRONIZATION_HPP

#include <WIFIMAC/convergence/IRxStartEnd.hpp>
#include <WIFIMAC/management/SINRInformationBase.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Dropper.hpp>
#include <WNS/Birthmark.hpp>
#include <WNS/events/CanTimeout.hpp>
#include <WNS/PowerRatio.hpp>

#include <WNS/ldk/probe/Probe.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>

namespace wifimac { namespace convergence {

    class FrameSynchronizationCommand:
        public wns::ldk::EmptyCommand
    {
    };

    enum SyncStateType {
        Idle,
        Synchronized,
        Garbled,
        waitForFinalDelivery
    };

	/**
	 * @brief FrameSynchronization models the capture effect in OFDM-Receivers
	 *
	 * To receive a frame successfully, the OFDM-receiver has to be synchronized
	 * to the transmission via the leading preamble. Hence, not only the SINR
	 * plays an important role for the packet error rate, but also if the
	 * synchronization has been successfully.
	 *
	 * In this FU, three scenarios are modeled: SenderFirst (SF),
	 * SenderLastClear (SLC), SenderLastGarbled (SLG).
     *
	 * - SF: The sender starts while no other transmissions are active, the
	 *       interfering tx starts afterwards.  Synchronization is successfull
	 *       if the SINR of the preamble is above idleCapture.
     *
	 * - SLC: The interferer starts first, and the receiver is successfully
	 *        synchronized. Then, a transmission is received with at least
	 *        slcCapture dB; this "captures" the receiver to the new sender.
     *
	 * - SLG: Same as in SLC, but the receiver is not successfully synchronized,
	 *        because the SINR is too low or it has missed the preamble due to
	 *        another, earlier, overlapping transmission. The "capture" to the
	 *        new transmission required slgCapture.
     *
	 * Usually, 0 <= idleCapture <= slcCapture <= slgCapture.  For more
	 * information, see Lee et al., "An Experimental Study on the Capture Effect
	 * in 802.11a"
	 */
    class FrameSynchronization:
        public wns::ldk::fu::Plain<FrameSynchronization, FrameSynchronizationCommand>,
        public wns::events::CanTimeout,
        public RxStartEndNotification,
        public wns::ldk::probe::Probe
    {
    public:
        FrameSynchronization(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
        virtual ~FrameSynchronization();
    private:
        // CompoundHandlerInterface
        void doSendData(const wns::ldk::CompoundPtr& compound);
        void doOnData(const wns::ldk::CompoundPtr& compound);
        void onFUNCreated();
        bool doIsAccepting(const wns::ldk::CompoundPtr& compound) const;
        void doWakeup();

        // CanTimeout Interface
        void onTimeout();

        void processPreamble(const wns::ldk::CompoundPtr& compound);
        void processPSDU(const wns::ldk::CompoundPtr& compound);
        void failedSyncToNewPreamble(wns::simulator::Time fExDur);
        void syncToNewPreamble(const wns::simulator::Time fExDur, const wns::service::dll::UnicastAddress transmitter);


        wns::logger::Logger logger;
        SyncStateType curState;
        wns::service::dll::UnicastAddress synchronizedToAddress;
        const wns::Ratio slcCapture;
        const wns::Ratio slgCapture;
        const wns::Ratio idleCapture;
        const wns::Ratio detectionThreshold;
        wns::simulator::Time lastFrameEnd;

        const std::string managerName;
        const std::string crcCommandName;
        const std::string phyUserCommandName;
        const std::string errorModellingCommandName;
        const std::string sinrMIBServiceName;

        /**
		 * @brief Probing of the PSDU success rate
		 */
        wns::probe::bus::ContextCollectorPtr successRateProbe;
        /** @brief Probing the sinr of received MPDUs */
        wns::probe::bus::ContextCollectorPtr sinrProbe;

        /** @brief Probing the per of received MPDUs */
        wns::probe::bus::ContextCollectorPtr perProbe;

        wifimac::management::SINRInformationBase* sinrMIB;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;
    };
} // end namespace convergence
} // end namespace wifimac

#endif // ifndef WIFIMAC_CONVERGENCE_FRAMESYNCHRONIZATION_HPP
