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

namespace wifimac { namespace lowerMAC {
	/** 
     * @brief Interface class of FUs for which TXOP controls outgoing data flow
     *	 FU should only process outgoing data with transmission duration smaller
     *	 than the set duration value
     */
	class ITXOPWindow
	{
	public:
		/**
         * @brief set the time window for transmissions
         * sets the max duration for transmission of outgoing compounds
         * and activates FU for process outgoing data.
         * The FU itself is responsible for deactivating itself after reaching
         * the time limit
         */
		virtual void setDuration(wns::simulator::Time duration) = 0;

		/**
         * @brief get time portion of duration window actually used
         * this function returns the time that is actually used for transmitting
         * outgoing compounds, given the passed time limit
         */
		virtual wns::simulator::Time getActualDuration(wns::simulator::Time duration) = 0;

		/**
         * @brief returns first outgoing compound
         * the first of the compounds that fit into the time window is passed
         * (if any)
         */
		virtual const wns::ldk::CompoundPtr hasSomethingToSend() const = 0;	
	};
} // lowerMAC
} // wifimac
#endif // WIFIMAC_LOWERMAC_ITXOPWINDOW_HPP
