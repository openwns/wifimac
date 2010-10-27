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

#ifndef WIFIMAC_PATHSELECTION_BEACONLINKQUALITYMEASUREMENT_HPP
#define WIFIMAC_PATHSELECTION_BEACONLINKQUALITYMEASUREMENT_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/pathselection/Metric.hpp>
#include <WIFIMAC/pathselection/IPathSelection.hpp>
#include <WIFIMAC/management/Beacon.hpp>
#include <WIFIMAC/management/SINRInformationBase.hpp>

#include <WNS/ldk/Command.hpp>
#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/probe/Probe.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>

#include <WNS/events/PeriodicTimeout.hpp>
#include <WNS/events/CanTimeout.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/Observer.hpp>
#include <WNS/distribution/Uniform.hpp>
#include <WNS/SlidingWindow.hpp>

namespace wifimac { namespace pathselection {

    /**
     * @brief Mapping of address to success ratio
     */
    typedef wns::container::Registry<wns::service::dll::UnicastAddress, double> Address2SuccessRateMap;
    /**
     * @brief Mapping of address to SINR
     */
    typedef wns::container::Registry<wns::service::dll::UnicastAddress, wns::Ratio> Address2RatioMap;

    /**
     * @brief Command to exchange link quality measurements in a beacon
     * information element
     */
    class BeaconLinkQualityMeasurementCommand :
        public wns::ldk::Command
    {
    public:
        struct { } local;

        struct {
            /**
             * @brief The own beacon transmission interval
             */
            wns::simulator::Time interval;

            /**
             * @brief Enumeration of the success rates of the received beacons
             */
            Address2SuccessRateMap rxBeaconSuccessRates;

            /**
             * @brief Enumeration of the average SINR of the received beacons
             */
            Address2RatioMap rxBeaconSINRs;
        } peer;

        struct { } magic;
    };

    class BeaconLinkQualityMeasurement;

    /**
	 * @brief Stores the (averaged) quality for one link as measured
	 */
    class BroadcastLinkQuality:
        public wns::events::CanTimeout
    {
    public:
        /** @brief Creator*/
        BroadcastLinkQuality(const wns::pyconfig::View& config_,
                             BeaconLinkQualityMeasurement* parent_,
                             wns::service::dll::UnicastAddress peerAddress_,
                             wns::service::dll::UnicastAddress myAddress_,
                             wns::simulator::Time interval_);
        /** @brief Returns the received beacon success rate */
        double
        getSuccessRate();

        /** @brief Returns the average sinr out of the sinr MIB */
        wns::Ratio
        getAverageSINR();

        /** @brief Indicates the reception of a new beacon from peer */
        void
        newBeacon(wns::simulator::Time interval);

        /** @brief Indicates a new measurement of the Me->Peer link */
        Metric
        newPeerMeasurement(double peerSuccessRate, wns::Ratio peerSINR);

        /** @brief Indicates the reception of a beacon from peer without a
         *  measurement
         */
        void
        deadPeerMeasurement();

        /** @brief Returns if beacons have been received at all */
        bool
        isActive() const;

    private:
        /**
         * @brief Is called if no beacon was received after 1.5 beacon intervalls
         */
        void
        onTimeout();

        /**
         * @brief Converts SINR to Mbps
         *
         * TODO: Exchange against call to protocol calculator
         */
        double
        getBestRate(wns::Ratio sinr);

        /**
         * @brief Store the config
         */
        const wns::pyconfig::View config;

        /** @brief Callback-pointer to the parent*/
        BeaconLinkQualityMeasurement* parent;

        /** @brief remote address*/
        const wns::service::dll::UnicastAddress peerAddress;

        /** @brief My address */
        wns::service::dll::UnicastAddress myAddress;

        /** @brief The last indicated beacon interval */
        wns::simulator::Time curInterval;

        /** @brief Storage of the success rate peer --> me */
        wns::SlidingWindow successRate;

        /** @brief Counts how many beacons are missed in a row, determines if
		 * the link is active */
        int missedBeaconsInRow;

        /** @brief Remember if the link is created in the routing table, it has
         * to be constantly active to remain created */
        bool linkCreated;

        /** @brief Pointer to the path selection entity */
        IPathSelection* ps;

        /** @brief Pointer to the node's SINR informatin base */
        wifimac::management::SINRInformationBase* sinrMIB;

        /** @brief frame size for metric calculation */
        const Bit meanFrameSize;
        /** @brief expected duration of an ACK for metric calculation */
        const wns::simulator::Time maximumACKDuration;
        /** @brief Slot duration for metric calculation */
        const wns::simulator::Time slotDuration;
        /** @brief SIFS duration for metric calculation */
        const wns::simulator::Time sifsDuration;
        /** @brief Preamble duration for metric calculation */
        const wns::simulator::Time preambleDuration;
        /** @brief scaling factor for metric calculation */
        const double scalingFactor;
        /** @brief Maximum number of missed beacons befor a link is closed*/
        const int maxMissedBeacons;
    };

    /**
	 * @brief The LQM measures the average one-way flight-time of all current active
	 *    links and provides this measure as a metric for the path-selection
	 *    FU.
	 *
	 * It does so by sending either probe-packets with a fixed length or
	 * by piggybacking its required control information (i.e. a timestamp) to
	 * frames which have to be transmitted anyways.
	 */
    class BeaconLinkQualityMeasurement :
        public wns::ldk::fu::Plain<BeaconLinkQualityMeasurement, BeaconLinkQualityMeasurementCommand>,
        public wns::ldk::probe::Probe
    {
    public:
        /** @brief Constructor */
        BeaconLinkQualityMeasurement(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        /** @brief Destructor */
        virtual 
        ~BeaconLinkQualityMeasurement();

        /**
		 * @brief Indicates a change in the link cost
		 */
        void newLinkCost(wns::service::dll::UnicastAddress rx, Metric cost);

        /** @brief The logger
         *
         *  Has to be public so that the BroadcastLinkQuality entities can use it*/
        wns::logger::Logger logger;

    private:
        /** @brief Forward request to lower FU */
        virtual bool doIsAccepting(const wns::ldk::CompoundPtr& _compound) const;

        /** @brief Add Information Element to beacon */
        virtual void doSendData(const wns::ldk::CompoundPtr& _compound);

        /** @brief Forward call to upper FU */
        virtual void doWakeup();

        /**
         * @brief Parse and process incoming beacon for Information Element
         *
         * - Store SINR and success rate of the incoming beacons
         * - If an IE is in the beacon, parse the information about the
         *   success/SINR of the own beacons previously transmitted and received
         *   by the peer.
         */
        virtual void doOnData(const wns::ldk::CompoundPtr& _compound);

        /** @brief Initialization */
        virtual void onFUNCreated();

        /**
		 * @brief SDU and PCI size calculation for probe-frames and frames with piggybacked lqm
		 */
        void calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const;

        /** @brief The configuration */
        const wns::pyconfig::View config;

        /** @brief The own beacon interval to be stored in each IE */
        wns::simulator::Time beaconInterval;

        /**
		 * @brief Pointer to the current path-selection algorithm for link-metric updates
		 */
        IPathSelection* ps;

        /** @brief Map of address to BroadcastLinkQuality */
        typedef wns::container::Registry<wns::service::dll::UnicastAddress, BroadcastLinkQuality*> adr2qualityMap;

        /**
		 * @brief Stores and manages the linkQualities for the links to all neighboring nodes
		 */
        adr2qualityMap linkQualities;

        /** @brief My own MAC address */
        wns::service::dll::UnicastAddress myMACAddress;

        /** @brief Name of PHY user commands to read the SINR */
        const std::string phyUserCommandName;

        /** @brief Probing the power of received beacons */
        wns::probe::bus::ContextCollectorPtr receivedPower;

        /** @brief Probing success rate of received beacons */
        wns::probe::bus::ContextCollectorPtr peerMeasurement;

        /** @brief Probing the link cost */
        wns::probe::bus::ContextCollectorPtr linkCost;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;
    };

}}

#endif

