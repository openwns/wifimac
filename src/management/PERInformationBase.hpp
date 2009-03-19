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

#ifndef WIFIMAC_MANAGEMENT_PERINFORMATIONBASE_HPP
#define WIFIMAC_MANAGEMENT_PERINFORMATIONBASE_HPP

#include <WNS/ldk/ManagementServiceInterface.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/SlidingWindow.hpp>
#include <WNS/ldk/Compound.hpp>
#include <WNS/service/dll/Address.hpp>
#include <WNS/ldk/arq/statuscollector/Interface.hpp>

namespace wifimac { namespace management {

    class PERInformationBase:
        public wns::ldk::ManagementService
    {
    public:
        PERInformationBase( wns::ldk::ManagementServiceRegistry*, const wns::pyconfig::View& config );
        virtual ~PERInformationBase() {};

        void reset(const wns::service::dll::UnicastAddress target);

        void onSuccessfullTransmission(const wns::service::dll::UnicastAddress target);
        void onFailedTransmission(const wns::service::dll::UnicastAddress target);

        bool knowsPER(const wns::service::dll::UnicastAddress target) const;
        double getPER(const wns::service::dll::UnicastAddress target) const;

    private:
        void
        onMSRCreated();

        typedef wns::container::Registry<wns::service::dll::UnicastAddress, wns::SlidingWindow*> slidingWindowMap;
        slidingWindowMap perHolder;

        wns::logger::Logger logger;
        const simTimeType windowSize;
        const int minSamples;
    };
} // management
} // wifimac

#endif
