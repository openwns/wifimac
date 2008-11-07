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

#ifndef WIFIMAC_PATHSELECTION_FORWARDINGCOMMAND_HPP
#define WIFIMAC_PATHSELECTION_FORWARDINGCOMMAND_HPP

#include <WNS/ldk/Command.hpp>
#include <WNS/service/dll/Address.hpp>


namespace wifimac { namespace pathselection {

	class ForwardingCommand :
		public wns::ldk::Command
	{
	public:
		ForwardingCommand()
			{
				peer.TTL = 0;
				peer.toDS = false;
				peer.fromDS = false;
				peer.addressExtension = false;
				peer.finalDestination = wns::service::dll::UnicastAddress();
				peer.originalSource = wns::service::dll::UnicastAddress();
				peer.meshSource = wns::service::dll::UnicastAddress();
				peer.meshDestination = wns::service::dll::UnicastAddress();

				magic.hopCount = 0;
				magic.isUplink = false;

				magic.path.clear();

			}

		struct {
		} local;

		/**
		 * @brief 6-field Addressing in a mesh
		 *
		 * originalSource -> meshSource -> uc.sourceMACAddress
		 *      --> (current transmission) -->
		 * uc.targetMACAddress -> meshDestination -> finalDestination
		 *
		 */
		struct {
			int TTL;
			bool toDS;
			bool fromDS;
			bool addressExtension;
			// only used if addressExtension = true
			// Source of the frame in the mesh
			wns::service::dll::UnicastAddress meshSource;
			// Sink of the frame in the mesh
			wns::service::dll::UnicastAddress meshDestination;

			// Source of the frame
			wns::service::dll::UnicastAddress originalSource;
			// Sink of the frame
			wns::service::dll::UnicastAddress finalDestination;

		} peer;

		struct {
			std::vector<wns::service::dll::UnicastAddress> path;
			int hopCount;
			bool isUplink;
		} magic;
	};
}
}
#endif // WIFIMAC_PATHSELECTION_FORWARDINGCOMMAND_HPP
