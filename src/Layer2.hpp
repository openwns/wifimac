/******************************************************************************
 * WiFiMAC (IEEE 802.11 and its alphabet soup)                                *
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

#ifndef WIFIMAC_LAYER2_HPP
#define WIFIMAC_LAYER2_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/ldk/ldk.hpp>
#include <WNS/ldk/fun/Main.hpp>
#include <WNS/service/phy/ofdma/DataTransmission.hpp>
#include <WNS/logger/Logger.hpp>

#include <DLL/Layer2.hpp>

namespace dll {
    class StationManager;
}

namespace wifimac
{
    /**
	 * @brief Basis component of the WiFiMAC
     *
     * Allows multiple transceivers, each consisting of a lower
	 * MAC, a convergence and a OFDM-based radio.
	 *
	 * Each transceiver \b must contain the FU wifimac::lowerMAC::Manager. This FU registers when
	 * \c onFUNCreated() is called at the Layer2, together with
	 * its unique MAC-address of type
	 * wns::service::dll::UnicastAddress. Furthermore, this manager hides
	 * the multi-transceiver structure from all other FUs from its
	 * transceivers and provides methods to get the appropriate MAC address
	 * and to build frames emerging from this transceiver.
	 *
	 */
    class Layer2:
        public dll::Layer2
	{
		typedef wns::container::Registry<wns::service::dll::UnicastAddress,
                                         wifimac::lowerMAC::Manager*> ManagerRegistry;
	public:
		Layer2(wns::node::Interface*, const wns::pyconfig::View&);
		virtual ~Layer2() {};

		// ComponentInterface
		virtual void onNodeCreated();
		virtual void onWorldCreated();

		/** @brief Registration of transceiver management entity*/
        void registerManager(wifimac::lowerMAC::Manager* manager,
                             wns::service::dll::UnicastAddress address);

		/** @brief Queries the registered transceiver list */
		bool isTransceiverMAC(wns::service::dll::UnicastAddress address);

	private:
		// disallow copy constructor
		Layer2(const Layer2&);
		// disallow assignment
		Layer2& operator=(const Layer2&);

		virtual void doStartup();

		/** @brief Logging */
		wns::logger::Logger logger_;

		/** @brief Holds all management entities */
		ManagerRegistry managers_;
	};
}

#endif // NOT defined WIFIMAC_LAYER2_HPP


