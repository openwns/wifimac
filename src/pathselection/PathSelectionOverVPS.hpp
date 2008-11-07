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

#ifndef WIFIMAC_PATHSELECTION_PATHSELECTION_HPP
#define WIFIMAC_PATHSELECTION_PATHSELECTION_HPP

#include <WIFIMAC/pathselection/VirtualPathSelection.hpp>
#include <WIFIMAC/pathselection/IPathSelection.hpp>

#include <DLL/services/control/Association.hpp>

#include <WNS/ldk/ManagementServiceInterface.hpp>
#include <WNS/logger/Logger.hpp>

#include <WNS/service/dll/Address.hpp>


namespace wifimac { namespace pathselection {

	/**
	 * @brief Implementation of the MP's path selection using the virtual path
	 *        selection service, i.e. with global information.
	 *
	 *        Essentially, all calls are relayed to TheVPSService::Instance().getVPS()->...
	 */
	class PathSelectionOverVPS :
        public IPathSelection,
		public wns::ldk::ManagementService,
		public dll::services::control::AssociationObserver
	{
	public:
		PathSelectionOverVPS( wns::ldk::ManagementServiceRegistry*, const wns::pyconfig::View& config );
		virtual ~PathSelectionOverVPS() {};

		/**
		 * @brief Implementation of the PathSelectionInterface
		 */
		virtual wns::service::dll::UnicastAddress
		getNextHop(const wns::service::dll::UnicastAddress current,
				   const wns::service::dll::UnicastAddress finalDestination);
		virtual bool
		isMeshPoint(const wns::service::dll::UnicastAddress address) const;
		virtual bool
		isPortal(const wns::service::dll::UnicastAddress address) const;
		virtual wns::service::dll::UnicastAddress
		getProxyFor(const wns::service::dll::UnicastAddress address);
		virtual wns::service::dll::UnicastAddress
		getPortalFor(const wns::service::dll::UnicastAddress address);
		virtual void
		registerProxy(const wns::service::dll::UnicastAddress server,
					  const wns::service::dll::UnicastAddress client);
		virtual void
		registerMP(const wns::service::dll::UnicastAddress address);
		virtual void
		registerPortal(const wns::service::dll::UnicastAddress address,
					   dll::APUpperConvergence* apUC);

		
		virtual void
		deRegisterProxy(const wns::service::dll::UnicastAddress server,
						const wns::service::dll::UnicastAddress client);
		virtual void
		createPeerLink(const wns::service::dll::UnicastAddress myself,
					   const wns::service::dll::UnicastAddress peer,
					   const Metric linkMetric = Metric(1));
		virtual void
		updatePeerLink(const wns::service::dll::UnicastAddress myself,
					   const wns::service::dll::UnicastAddress peer,
					   const Metric linkMetric = Metric(1));
		virtual void
		closePeerLink(const wns::service::dll::UnicastAddress myself,
					  const wns::service::dll::UnicastAddress peer);

	private:
		void
		onMSRCreated();

		/**
		 * @brief Implementation of dll::services::control::AssociationObserver
		 */
		void onAssociated(const wns::service::dll::UnicastAddress clientAdr,
				  const wns::service::dll::UnicastAddress serverAdr);
		void onDisassociated(const wns::service::dll::UnicastAddress clientAdr,
				     const wns::service::dll::UnicastAddress serverAdr);

		wns::logger::Logger logger;
		std::string upperConvergenceName;
	};
} // mac
} // wifimac

#endif // WIFIMAC_SCHEDULER_HPP
