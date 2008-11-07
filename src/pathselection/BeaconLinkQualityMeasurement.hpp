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

#include <WIFIMAC/pathselection/Metric.hpp>
#include <WIFIMAC/pathselection/IPathSelection.hpp>
#include <WIFIMAC/management/Beacon.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
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

	typedef wns::container::Registry<wns::service::dll::UnicastAddress, double> Address2SuccessRateMap;
	typedef wns::container::Registry<wns::service::dll::UnicastAddress, wns::Ratio> Address2RatioMap;

	class BeaconLinkQualityMeasurementCommand :
		public wns::ldk::Command
	{
	public:
		struct {
		} local;

		struct {
			wns::simulator::Time interval;
			Address2SuccessRateMap rxBeaconSuccessRates;
			Address2RatioMap rxBeaconSINRs;
		} peer;

		struct {
		} magic;
	};

	class BeaconLinkQualityMeasurement;

	/**
	 * @brief Stores the (averaged) quality for one link
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
			measurement */
		void
		deadPeerMeasurement();

		/** @brief Returns if beacons have been received at all */
		bool
		isActive() const;

	private:
		void
		onTimeout();
		double
		getBestRate(wns::Ratio sinr);

		const wns::pyconfig::View config;

		/** @brief Callback-pointer to the parent*/
		BeaconLinkQualityMeasurement* parent;

		/** @brief remote address*/
		const wns::service::dll::UnicastAddress peerAddress;
		wns::service::dll::UnicastAddress myAddress;

		wns::simulator::Time curInterval;

		/** @brief Storage of the success rate peer --> me */
		wns::SlidingWindow successRate;
		Metric oldLinkCostTxAvg;

		/** @brief Counts how many beacons are missed in a row, determines if
		 * the link is active */
		int missedBeaconsInRow;

		/** @brief Remember if the link is created in the routing table, it has
		 * to be constantly active to remain created */
		bool linkCreated;

		IPathSelection* ps;
        wifimac::management::SINRInformationBase* sinrMIB;

		const Bit meanFrameSize;
		const wns::simulator::Time expectedAckDuration;
		const wns::simulator::Time slotDuration;
		const wns::simulator::Time sifsDuration;
		const wns::simulator::Time preambleDuration;
		const double scalingFactor;
		const int maxMissedBeacons;

        wns::Ratio lastPeerSINR;
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
		BeaconLinkQualityMeasurement(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
		virtual ~BeaconLinkQualityMeasurement();

		/**
		 * @brief Indicates a change in the link cost
		 */
		void newLinkCost(wns::service::dll::UnicastAddress rx, Metric cost);
		wns::logger::Logger logger;

	private:
		/**
		 * @brief FunctionalUnit / CompoundHandlerInterface
		 */
		virtual bool doIsAccepting(const wns::ldk::CompoundPtr& _compound) const;
		virtual void doSendData(const wns::ldk::CompoundPtr& _compound);
		virtual void doWakeup();
		virtual void doOnData(const wns::ldk::CompoundPtr& _compound);

		virtual void onFUNCreated();

		/**
		 * @brief SDU and PCI size calculation for probe-frames and frames with piggybacked lqm
		 */
		void calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const;

		const wns::pyconfig::View config;
		wns::simulator::Time beaconInterval;

		/**
		 * @brief Pointer to the current path-selection algorithm for link-metric updates
		 */
		IPathSelection* ps;

		/**
		 * @brief Stores and manages the linkQualities for the links to all neighboring nodes
		 */
		typedef wns::container::Registry<wns::service::dll::UnicastAddress, BroadcastLinkQuality*> adr2qualityMap;
		adr2qualityMap linkQualities;

		wns::service::dll::UnicastAddress myMACAddress;

		const std::string phyUserCommandName;

        wns::probe::bus::ContextCollectorPtr receivedPower;
        wns::probe::bus::ContextCollectorPtr peerMeasurement;
        wns::probe::bus::ContextCollectorPtr linkCost;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;
    };

}}

#endif

