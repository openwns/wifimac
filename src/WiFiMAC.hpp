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

#ifndef WIFIMAC_HPP
#define WIFIMAC_HPP

#include <WNS/module/Module.hpp>
#include <WNS/ldk/ldk.hpp>

/**
 * @defgroup group_modules_wifimac WiFiMAC
 * @ingroup group_modules
 *
 * @brief Provides a data link layer and phy model according to IEEE 802.11
 * standart, also known as WiFi or WLAN
 *
 * The WiFiMAC is partitioned into three different sections:
 *
 * @li The Higher MAC, also often named layer 2.5, responsible for the path
 * selection in a multi-hop IEEE 802.11 network and coordinating one or more
 * lower MACs.
 * @li The Lower MAC, implementing the IEEE 802.11 Distributed Contention
 * Function (DCF) and derivates.
 * @li The Convergence, which models the IEEE 802.11 physical layer.
 *
 * Additional functionality is provided by the management plane and helper
 * functionalities.
 *
 * The basis component of is wifimac::MultiRadioLayer2. It is derived from
 * dll::Layer2 and allows for multiple transceivers, each consisting of a lower
 * MAC, a convergence and a OFDM-based radio.
 *
 * @section HigherMAC Higher MAC
 *
 * The Higher MAC provides the wifimac::pathselection functionality, including measurement
 * of the link metric.
 *
 * @section LowerMAC Lower MAC
 *
 * The Lower MAC is all about the Distributed Contention Function (DCF) of IEEE
 * 802.11 and possible enhancements: wifimac::scheduler
 *
 * @section Convergence Convergence
 *
 * wifimac::convergence provides FUs to access the OFDM physical layer, but also
 * incorporates
 * @li Error modelling, including synchronization and
 * @li Interfaces to signal the current channel state to the MAC
 *
 * @section Add Additional functionality
 *
 * wifimac::helper incorporates functions for dll::CompoundFilter,
 * contextprovider and keys.
 *
 * wifimac::management contains functions for the management plane. Most
 * importantly the wifimac::management::Beacon FU for beacon transmission and
 * reception plus association; Furthermore, the management information bases for
 * PER measurement (wifimac::management::PERInformationBase) and SINR
 * measurement (wifimac::management::SINRInformationBase)
 *
 */

namespace dll {
	class StationManager;
}

namespace wifimac
{
	/**
	 * @brief Anchor of the library.
	 *
	 * Start up and shut down are coordinated from here
	 * WNS-Core will use an instance of this class to start up and shut down the
	 * library. Therefor it calls GetModuleInterface() which returns a pointer
	 * to an instance of this class. The method StartUp() is called
	 * subsequently.
	 */

	class WiFiMAC :
		public wns::module::Module<WiFiMAC>
	{
	public:
		WiFiMAC(const wns::pyconfig::View& config);

		// Module interface
		virtual void configure();
		virtual void startUp();
		virtual void shutDown();

	private:
		wns::logger::Logger logger;

	};
}

#endif // NOT defined WIFIMAC_HPP


