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

#ifndef WIFIMAC_CONVERGENCE_INETWORKALLOCATIONVECTOR_HPP
#define WIFIMAC_CONVERGENCE_INETWORKALLOCATIONVECTOR_HPP

#include <WNS/Subject.hpp>
#include <WNS/service/dll/Address.hpp>

namespace wifimac { namespace convergence {

    /**
     * @brief Indication of the Network Allocation Vector status (busy/idle)
     *
     * The NAV might have a different value than the channel status
     * notification! A NAV is set to true only for "overheared" transmissions,
     * i.e. nor for own transmissions neither for transmissions for which the
     * node is the receiver!
     *
     * Application example is e.g. the RTS/CTS: According to the standard, the
     * receiver replies with the CTS only if the NAV is not set. Hence, it must
     * not set the NAV upon the reception of the RTS which is addressed to it.
     */
	class INetworkAllocationVector
	{
	public:
		virtual ~INetworkAllocationVector(){};

		/*** @brief called by ChannelState, implemented by observers on NAV ***/
		virtual void onNAVBusy(const wns::service::dll::UnicastAddress setter) = 0;
		virtual void onNAVIdle() = 0;
	};

    class NAVNotification :
        virtual public wns::Subject<INetworkAllocationVector>
    {
    public:
        // @brief functor for INetworkAllocationVector::onChangedNAV calls
        struct OnChangedNAV
        {
            OnChangedNAV(const bool _newNAV, const wns::service::dll::UnicastAddress _setter):
                newNAV(_newNAV),
                setter(_setter)
                {}

            void operator()(INetworkAllocationVector* nav)
				{
					// The functor calls the onChannelBusy/onChannelIdle implemented by the
					// Observer
					if (newNAV)
						nav->onNAVBusy(setter);
					else
						nav->onNAVIdle();
				}
		private:
			bool newNAV;
            wns::service::dll::UnicastAddress setter;
		};

	};


}
}

#endif // not defined WIFIMAC_CONVERGENCE_INETWORKALLOCATIONVECTOR_HPP
