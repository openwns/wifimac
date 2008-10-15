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

#ifndef WIFIMAC_PATHSELECTION_PATHSELECTIONINTERFACE_HPP
#define WIFIMAC_PATHSELECTION_PATHSELECTIONINTERFACE_HPP

#include <WIFIMAC/pathselection/Metric.hpp>
#include <WNS/service/dll/Address.hpp>

#include <DLL/UpperConvergence.hpp>


namespace wifimac { namespace pathselection {

	/**
	 * @brief Interface of the path selection management service
	 */
	class IPathSelection
	{
	public:
		virtual
		~IPathSelection()
			{};
		/**
		 * @brief Query the path selection table
		 */
		virtual wns::service::dll::UnicastAddress
		getNextHop(const wns::service::dll::UnicastAddress current,
				   const wns::service::dll::UnicastAddress finalDestination) = 0;

		/**
		 * @brief Returns true if address is a known MP, false otherwise
		 */
		virtual bool
		isMeshPoint(const wns::service::dll::UnicastAddress address) const = 0;

		/**
		 * @brief Returns true if address is a known Portal, false otherwise
		 */
		virtual bool
		isPortal(const wns::service::dll::UnicastAddress address) const = 0;

		/**
		 * @brief Returns the proxy MP for address if address is a known
		 *        client of a known MP; invalid address otherwise
		 */
		virtual wns::service::dll::UnicastAddress
		getProxyFor(const wns::service::dll::UnicastAddress address) = 0;

        /**
		 * @brief Returns the Portal for address if address is a known
		 *        client of a known Portal (which is connected to the RANG); 
		 *        invalid address otherwise
		 */
		virtual wns::service::dll::UnicastAddress
		getPortalFor(const wns::service::dll::UnicastAddress address) = 0;

        /**
		 * @brief Registers client as being proxied by server
		 */
		virtual void
		registerProxy(const wns::service::dll::UnicastAddress server,
					  const wns::service::dll::UnicastAddress client) = 0;
		/**
		 * @brief Register MP
		 */
		virtual void
		registerMP(const wns::service::dll::UnicastAddress address) = 0;

		/**
		 * @brief Register Portal
		 */
		virtual void
		registerPortal(const wns::service::dll::UnicastAddress address,
					   dll::APUpperConvergence* apUC) = 0;

		/**
		 * @brief Deregisters client as being proxied by server
		 */
		virtual void
		deRegisterProxy(const wns::service::dll::UnicastAddress server,
						const wns::service::dll::UnicastAddress client) = 0;

		/**
		 * @brief Setup a new unidirectional link
		 */
		virtual void
		createPeerLink(const wns::service::dll::UnicastAddress myself,
					   const wns::service::dll::UnicastAddress peer,
					   const Metric linkMetric = Metric(1)) = 0;
		/**
		 * @brief Update a unidirectional link, throw error if the link does not exist
		 */
		virtual void
		updatePeerLink(const wns::service::dll::UnicastAddress myself,
					   const wns::service::dll::UnicastAddress peer,
					   const Metric linkMetric = Metric(1)) = 0;

		/**
		 * @brief Tear-down of unidirectional link, throw error if the link does not exist
		 */
		virtual void
		closePeerLink(const wns::service::dll::UnicastAddress myself,
					  const wns::service::dll::UnicastAddress peer) = 0;

	};

} // pathselection
} // wifimac

#endif // WIFIMAC_SCHEDULER_HPP
