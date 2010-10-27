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

#ifndef WIFIMAC_CONVERGENCE_CHANNELSTATE_HPP
#define WIFIMAC_CONVERGENCE_CHANNELSTATE_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/convergence/IChannelState.hpp>
#include <WIFIMAC/convergence/INetworkAllocationVector.hpp>
#include <WIFIMAC/convergence/ITxStartEnd.hpp>
#include <WIFIMAC/convergence/IRxStartEnd.hpp>
#include <WIFIMAC/FrameType.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>
#include <WNS/ldk/Command.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/events/CanTimeout.hpp>
#include <WNS/service/phy/ofdma/Notification.hpp>
#include <WNS/service/phy/ofdma/Handler.hpp>
#include <WNS/Observer.hpp>
#include <WNS/Functor.hpp>
#include <WNS/ldk/crc/CRC.hpp>

#include <WNS/events/PeriodicTimeout.hpp>
#include <WNS/ldk/probe/Probe.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>
#include <WNS/SlidingWindow.hpp>

namespace wifimac { namespace lowerMAC {
    class Manager;
}}

namespace wifimac { namespace convergence {

    class ChannelStateCommand :
        public wns::ldk::EmptyCommand
    {
    };

    /**
	 * @brief Summarizes the channel state information, using several indicators
	 *
	 * IEEE 802.11 defines the following channel state indicators:
     *
	 * - Raw energy detection: Any signal above a given threshold is
     *   identified as a busy channel
     *
	 * - PHY carrier sense: Any valid OFDM signal above a given threshol is
     *   identified as a busy channel
     *
	 * - PHY packet length: The channel is busy for the length of the frame as
     *   indicated in the PLCP header
     *
     * - Network Allocation Vector: The channel is busy for the length of the
	 *   NAV as indicated in any received MAC-Header Additionally, the channel
	 *   is busy during own transmissions.
	 */
    class ChannelState :
        public wns::ldk::fu::Plain<ChannelState, ChannelStateCommand>,
        public wns::ldk::Processor<ChannelState>,
        public ChannelStateNotification,
        public NAVNotification,
        public wns::service::phy::ofdma::RSSHandler,
        public wns::Observer<wifimac::convergence::ITxStartEnd>,
        public wns::Observer<wifimac::convergence::IRxStartEnd>,
        public wns::events::CanTimeout,
        public wns::ldk::probe::Probe,
        public wns::events::PeriodicTimeout
    {

    public:
        ChannelState(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
        virtual ~ChannelState();

        void
        setCarrierSensingService(wns::service::Service* cs);

        void
        registerRSSObserver(wns::Observer<wns::service::phy::ofdma::CarrierSensing> *o) {rssObservers.push_back(o);}

    private:
        /** @brief Processor Interface Implementation */
        void
        processIncoming(const wns::ldk::CompoundPtr& compound);
        void
        processOutgoing(const wns::ldk::CompoundPtr& compound);

        virtual void
        onFUNCreated();

        /** @brief Notification from PHY on RSS change */
        virtual void
        onRSSChange(wns::Power newRSS);

        /** @brief observer TxStartEnd */
        void onTxStart(const wns::ldk::CompoundPtr& compound);
        void onTxEnd(const wns::ldk::CompoundPtr& compound);

        /** @brief observer RxStartEnd */
        void onRxStart(const wns::simulator::Time expRxDuration);
        void onRxEnd();
        void onRxError();


        virtual CS getCurrentChannelState() const;

        /** @brief canTimeoutInterface for all duration-based indicators */
        void onTimeout();

        /** @brief Check if the ChannelState has changed and an indication to the observers is required */
        virtual void checkNewCS();

        /** @brief Implementation of periodic timeout to evaluate the probe(s) */
        void periodically();

        bool isRTS(const wns::ldk::CompoundPtr& compound) const;

        /** @brief Small function for channel busy fraction probing */
        void probeChannelBusy();
        void probeChannelIdle();

        wns::pyconfig::View config;
        wns::logger::Logger logger;

        /** @brief units to be informed on RSS changes */
        std::vector<wns::Observer<wns::service::phy::ofdma::CarrierSensing>*> rssObservers;

        /**
		 * @brief identifies which indicators are switched on or off
		 *
		 * Carrier sense via
		 * - bool rawEnergyDetection: the detection of channel noise
		 * - bool phyCarrierSense: the detection of OFDM symbols
		 * - bool nav: the decoding of the MAC header & the enclosed frame exchange length
		 * - bool ownTx: own frame transmissions
         * - bool ownRx: own frame receptions
		 */
        struct ActiveIndicators {
            bool rawEnergyDetection;
            bool phyCarrierSense;
            bool nav;
            bool ownTx;
            bool ownRx;
        } activeIndicators;

        /**
		 * @brief required data to remember & compare the channel state
		 *
		 * last sensed
		 *  - wns::Power rawEnergy: raw channel noise
		 *  - wns::Power phyCarrierSense: OFDM symbol signal strength
		 *  - wns::simulator::Time latestDuration: Longest duration of (PhyLength/NAV/ownTx)
		 */
        struct Indicators {
            wns::Power rawEnergy;
            wns::Power phyCarrierSense;
            bool ownTx;
            bool ownRx;
        } indicators;

        /**
		 * @brief if the channel noise is above rawEnergyThreshold threshold, the channel is occupied
		 */
        const wns::Power rawEnergyThreshold;

        /**
		 * @brief if an OFDM transmission is detected above this threshold, the channel is occupied
		 */
        const wns::Power phyCarrierSenseThreshold;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;

        /**
		 * @brief last channel state for comparison
		 */
        CS lastCS;

        wns::simulator::Time latestNAV;

        bool waitForReply;

        wifimac::FrameType lastCommandOut;

        const std::string managerName;
        const std::string phyUserCommandName;
        const std::string rtsctsCommandName;
        const std::string txStartEndName;
        const std::string rxStartEndName;

        /**
         * @brief Frame/IFS durations
         */
        const wns::simulator::Time sifsDuration;

        /**
         * @brief Probe the channel busy fraction
         */
        wns::probe::bus::ContextCollectorPtr channelBusyFractionProbe;
        wns::simulator::Time channelBusyFractionMeasurementPeriod;
        wns::simulator::Time channelBusyTime;
        wns::simulator::Time channelBusySlotStart;
        wns::simulator::Time channelBusyLastChangeToBusy;

        wns::service::phy::ofdma::Notification* myCS;
    };


} // convergence
} // wifimac

#endif // WIFIMAC_CHANNELSTATE_HPP
