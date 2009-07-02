/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 5, D-52074 Aachen, Germany
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

#ifndef WIFIMAC_LOWERMAC_ITXOPWINDOW_HPP
#define WIFIMAC_LOWERMAC_ITXOPWINDOW_HPP

#include <WNS/simulator/Time.hpp>
#include <WNS/ldk/Compound.hpp>
#include <WNS/service/dll/Address.hpp>

namespace wifimac { namespace lowerMAC {
	/** Interface class for FUs which implement "peeking" ability for TXOP **/
	class ITXOPWindow
	{
	public:
		virtual wns::simulator::Time
		getNextTransmissionDuration() = 0;

		virtual wns::service::dll::UnicastAddress
		getNextReceiver() const = 0;	
	};
} // lowerMAC
} // wifimac
#endif // WIFIMAC_LOWERMAC_ITXOPWINDOW_HPP
