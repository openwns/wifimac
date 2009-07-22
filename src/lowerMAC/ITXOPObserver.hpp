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

#ifndef WIFIMAC_LOWERMAC_ITXOPOBSERVER_HPP
#define WIFIMAC_LOWERMAC_ITXOPOBSERVER_HPP

#include <WNS/ldk/Compound.hpp>

namespace wifimac { namespace lowerMAC {
	/** @brief TXOP observer on closed TXOP rounds
	* note that for triggered TXOP (= patient) an end of the TXOP round
	* might be indicated by calling the observer BEFORE the startTXOP() call returns
	* due to the way the FUN mechanisms work! (use the passed Bool to deal with this properly)
	*/
	class ITXOPObserver
	{
	public:
		virtual void
		onTXOPClosed() = 0;
	};
} // draftn
} // wifimac
#endif // WIFIMAC_LOWERMAC_ITXOPOBSERVER_HPP
