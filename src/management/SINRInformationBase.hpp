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

#ifndef WIFIMAC_MANAGEMENT_SINRINFORMATIONBASE_HPP
#define WIFIMAC_MANAGEMENT_SINRINFORMATIONBASE_HPP

#include <WNS/ldk/ManagementServiceInterface.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/PowerRatio.hpp>
#include <WNS/SlidingWindow.hpp>

#include <WNS/service/dll/Address.hpp>


namespace wifimac { namespace management {

	class SINRInformationBase:
		public wns::ldk::ManagementService
	{
	public:
		SINRInformationBase( wns::ldk::ManagementServiceRegistry*, const wns::pyconfig::View& config );
		virtual ~SINRInformationBase() {};


        /** @brief SINR for peer -> me (measured here) */
        void
        putMeasurement(const wns::service::dll::UnicastAddress tx,
                       const wns::Ratio sinr);

        bool
        knowsMeasuredSINR(const wns::service::dll::UnicastAddress tx);

        wns::Ratio
        getAverageMeasuredSINR(const wns::service::dll::UnicastAddress tx);


        /** @brief SINR for me -> peer (measured at peer) */
        void
        putPeerSINR(const wns::service::dll::UnicastAddress peer,
                    const wns::Ratio sinr);

        bool
        knowsPeerSINR(const wns::service::dll::UnicastAddress peer);

        wns::Ratio
        getAveragePeerSINR(const wns::service::dll::UnicastAddress peer);


	private:
		void
		onMSRCreated();

        typedef wns::container::Registry<wns::service::dll::UnicastAddress, wns::SlidingWindow*, wns::container::registry::DeleteOnErase> slidingWindowMap;
        slidingWindowMap measuredSINRHolder;

        typedef wns::container::Registry<wns::service::dll::UnicastAddress, wns::Ratio*> ratioMap;
        ratioMap peerSINRHolder;

		wns::logger::Logger logger;
        const simTimeType windowSize;

	};
} // management
} // wifimac

#endif
