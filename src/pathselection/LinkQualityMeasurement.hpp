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

#ifndef WIFIMAC_PATHSELECTION_LINKQUALITYMEASUREMENT_HPP
#define WIFIMAC_PATHSELECTION_LINKQUALITYMEASUREMENT_HPP

#include <WIFIMAC/pathselection/Metric.hpp>
#include <WIFIMAC/pathselection/PathSelectionInterface.hpp>
#include <WIFIMAC/management/LinkNotificationInterface.hpp>

#include <DLL/UpperConvergence.hpp>

#include <WNS/ldk/Command.hpp>
#include <WNS/ldk/fu/Plain.hpp>

#include <WNS/simulator/Time.hpp>
#include <WNS/events/PeriodicTimeout.hpp>
#include <WNS/events/CanTimeout.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/Observer.hpp>
#include <WNS/distribution/Uniform.hpp>
#include <WNS/SlidingWindow.hpp>

namespace wifimac { namespace pathselection {

	class LinkQualityMeasurementCommand :
		public wns::ldk::Command
	{
	public:
		struct {
		} local;

		struct {
			wns::service::dll::UnicastAddress srcMACAddress;
			wns::service::dll::UnicastAddress dstMACAddress;
			wns::simulator::Time txTime;
			wns::simulator::Time lastFrameFlightTime;
		} peer;

		struct {
			bool isPiggybacked;
		} magic;
	};
	
	class LinkQualityMeasurement;
	
	/**
	 * @brief Stores the (averaged) quality for one link
	 */
	class LinkQuality:
		public wns::events::CanTimeout,	
		public wns::events::PeriodicTimeout
	{
		public:
			/**
			 * @brief Creator
			 *
			 * @param _rx Address of the remote node of this the link
			 * @param _referenceFlightTime frame flight time which should result in Metric(1.0)
			 * @param _periodicity Period of the transmission of the probe-packets
			 * @param _parent Callback pointer
			 */
			LinkQuality(wns::service::dll::UnicastAddress _rx,
				    wns::simulator::Time _referenceFlightTime,
				    wns::simulator::Time _periodicity,
				    LinkQualityMeasurement* _parent);
			
			/**
			 * @brief Allows the parent to indicate a fresh received link quality indicator
			 *    by the remote node of this link
			 */
			void receivedLinkQualityIndicator(const LinkQualityMeasurementCommand* lqm);
			
			/**
			 * @brief Getter for the last FrameFlightTime which was received by the remote node.
			 *
			 * Note that this FrameFlightTime describes the cost (remote node)--->this, it is not
			 * used for the evaluation.
			 */
			wns::simulator::Time getLastRxFrameFlightTime() const;
			
			/**
			 * @brief Allows the parent to indicate that a piggybacked LQM has been transmitted
			 */
			void hasSendPiggybackedLQM();
		private:
			/**
			 * @brief Periodically transmission of the probe-frame
			 *
			 * Note that the probe-frame is not transmitted immediatelly, but after a random delay
			 * which is limited by period/2.
			 */
			void periodically();
			
			/**
			 * @brief Initiates the transmission of the probe frame after the random delay
			 */
			void onTimeout();
			
			/**
			 * @brief remote node of this link
			 */
			const wns::service::dll::UnicastAddress rx;
			
			/**
			 * @brief frame flight time which should result in Metric(1.0)
			 */
			const wns::simulator::Time referenceFlightTime;
			
			/**
			 * @brief Period of the transmission of the probe-packets
			 */
			wns::simulator::Time periodicity;
			
			/**
			 * @brief Callback-pointer to the parent
			 */
			LinkQualityMeasurement* parent;
			
			/**
			 * @brief Indicates if a lqm as a reply was received since the last transmission of my lqm.
			 *    If not, this indicates the my (or the reply) lqm got lost
			 */
			bool rxLastFrame;
			
			/**
			 * @brief Stores the frame flight time of the last received lqm, this indicates the quality
			 *   of the link (remote node)--->this
			 */
			wns::simulator::Time lastRxFrameFlightTime;
			
			/**
			 * @brief Storage of the link quality this--->(remote node)
			 */
			wns::SlidingWindow linkCostTx;
			
			/**
			 * @brief Stores the last link quality for comparison purpose
			 */
			Metric oldLinkCostTxAvg;
			
			/**
			 * @brief Random number generation for the random probe transmission delay
			 */
			wns::distribution::Uniform uniform;
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
	class LinkQualityMeasurement :
		public wns::ldk::fu::Plain<LinkQualityMeasurement, LinkQualityMeasurementCommand>,
		public wns::Observer<wifimac::management::LinkNotificationInterface>
	{
	public:
		LinkQualityMeasurement(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
		virtual ~LinkQualityMeasurement();
		
		/**
		 * @brief Initiates the transmission of a probe-packet
		 */
		void sendLinkMeasurement(wns::service::dll::UnicastAddress rx, wns::simulator::Time last);
		
		/**
		 * @brief Indicates a change in the link cost
		 */
		void newLinkCost(wns::service::dll::UnicastAddress rx, Metric cost);

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

		/**
		 * @brief observer for new link notifications / link updates
		 */
		void onLinkIndication(const wns::service::dll::UnicastAddress myself,
				      const wns::service::dll::UnicastAddress peer);
		
		
		const wns::pyconfig::View config;
		const Bit frameSize;
		const wns::simulator::Time referenceFlightTime;
		const wns::simulator::Time period;
		wns::logger::Logger logger;
		const std::string ucName;
		
		const bool doPiggybacking;
		int piggybackingPeriod;
		int numPiggybacked;
		
		/**
		 * @brief Pointer to the current path-selection algorithm for link-metric updates
		 */
		PathSelectionInterface* ps;
		
		/**
		 * @brief Stores and manages the linkQualities for the links to all neighboring nodes
		 */
		typedef wns::container::Registry<wns::service::dll::UnicastAddress, LinkQuality*, wns::container::registry::DeleteOnErase> adr2qualityMap;
		adr2qualityMap linkQualities;
		
		wns::service::dll::UnicastAddress myMACAddress;
		struct Friends
		{
			dll::UpperConvergence* uc;
		} friends;
	};

}}

#endif

