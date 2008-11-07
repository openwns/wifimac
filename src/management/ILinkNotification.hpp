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

#ifndef WIFIMAC_MANAGEMENT_ILINKNOTIFICATION_HPP
#define WIFIMAC_MANAGEMENT_ILINKNOTIFICATION_HPP

#include <WNS/Subject.hpp>
#include <WNS/service/dll/Address.hpp>

namespace wifimac { namespace management {

	/**
	 * @brief Provides an interface for indications of new/existing links.
	 */
	class ILinkNotification
	{
	public:
		virtual ~ILinkNotification() {};
		virtual void onLinkIndication(const wns::service::dll::UnicastAddress myself,
					      const wns::service::dll::UnicastAddress peer) = 0;
	};

	/**
	 * @brief Subject which can be observed to be indicated of new links are found / existing links are
	 *    confirmed.
	 */
	class LinkNotificator :
		virtual public wns::Subject<ILinkNotification>
	{
	public:
	// @brief functor for ILinkNotification::onLinkIndication calls
		struct OnLinkIndication
		{
			OnLinkIndication(const wns::service::dll::UnicastAddress _myself,
                             const wns::service::dll::UnicastAddress _peer):
                myself(_myself),
                peer(_peer)
                {}

				void operator()(ILinkNotification* lni)
				{
					// The functor calls the onLinkIndication implemented by the
					// Observer
					lni->onLinkIndication(myself, peer);
				}
			private:
				wns::service::dll::UnicastAddress myself;
				wns::service::dll::UnicastAddress peer;
			};
	};
}}

#endif
