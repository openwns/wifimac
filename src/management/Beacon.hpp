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

#ifndef WIFIMAC_MANAGEMENT_BEACON_HPP
#define WIFIMAC_MANAGEMENT_BEACON_HPP

#include <WIFIMAC/management/ILinkNotification.hpp>
#include <WIFIMAC/pathselection/IPathSelection.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>
#include <WNS/ldk/Command.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/events/PeriodicTimeout.hpp>
#include <WNS/events/CanTimeout.hpp>
#include <WNS/service/dll/Address.hpp>
#include <WNS/PowerRatio.hpp>
#include <WNS/pyconfig/Sequence.hpp>

#include <map>

namespace wifimac { namespace management {

    class BeaconCommand :
        public wns::ldk::Command
    {
    public:
        struct {} local;
        struct {
            std::string bssId;
        } peer;
        struct {} magic;
    };

    /**
	 * @brief Beacon transmission and reception
	 *
	 * The periodical beacon transmission and reception is required for several functions
	 *  - APs broadcast their presence
	 *  - STAs learn about active APs, select one and associate to it
	 *  - All nodes learn about their neighboring nodes and thus of possible links
	 */
	class Beacon :
		public wns::ldk::fu::Plain<Beacon, BeaconCommand>,
        public wns::ldk::Delayed<Beacon>,
		public LinkNotificator,
		public wns::events::PeriodicTimeout,
		public wns::events::CanTimeout
	{
	public:

		Beacon(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

		virtual
		~Beacon();

        /// Delayed interface realization
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);
        bool hasCapacity() const;
        const wns::ldk::CompoundPtr hasSomethingToSend() const;
        wns::ldk::CompoundPtr getSomethingToSend();


	private:
		/**
		 * @brief FunctionalUnit / CompoundHandlerInterface
		 */

		virtual void onFUNCreated();

		/**
		 * @brief PeriodicTimeout realisation, initiating a beacon transmission
		 *
		 * Note: Not used by STAs
		 */
		virtual void periodically();

		/**
		 * @brief CanTimeout realisation, performs the association to the beacon with the highest RSS
		 *
		 * Note: Only used by STAs
		 */
		virtual void onTimeout();

		/**
		 * @brief SDU and PCI size calculation for beacons
		 */
		void calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const;

		wns::pyconfig::View config;
		wns::logger::Logger logger;

        wns::ldk::CompoundPtr currentBeacon;

		const std::string phyUserCommandName;

		/**
		 * @brief How long shall a STA scan for beacons before it associates to one?
		 */
        wns::simulator::Time scanDuration;
        wns::pyconfig::Sequence scanFrequencies;
        wns::pyconfig::Sequence::iterator<double> freqIter;

        const int beaconPhyModeId;


        /**
         * @brief APs transmit beacons to bssId, STAs only associate to APs with
         *   'their' bssId
         */
        const std::string bssId;

        /**
		 * @brief Remember the reception strength of received beacons
		 */
        typedef std::map<wns::Power, wns::service::dll::UnicastAddress> power2adrMap;
        power2adrMap beaconRxStrength;
        typedef std::map<wns::service::dll::UnicastAddress, double> adr2frequencyMap;
        adr2frequencyMap bssFrequencies;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;
    };


} // mac
} // wifimac

#endif // WIFIMAC_SCHEDULER_HPP
