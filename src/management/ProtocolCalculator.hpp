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

#ifndef WIFIMAC_MANAGEMENT_PROTOCOLCALCULATOR_HPP
#define WIFIMAC_MANAGEMENT_PROTOCOLCALCULATOR_HPP

#include <WIFIMAC/management/protocolCalculatorPlugins/ErrorProbability.hpp>
#include <WIFIMAC/management/protocolCalculatorPlugins/FrameLength.hpp>
#include <WIFIMAC/management/protocolCalculatorPlugins/Duration.hpp>

#include <WNS/ldk/ManagementServiceInterface.hpp>
#include <WNS/logger/Logger.hpp>

namespace wifimac { namespace management {

    class ProtocolCalculator:
        public wns::ldk::ManagementService
    {
    public:
        ProtocolCalculator( wns::ldk::ManagementServiceRegistry*, const wns::pyconfig::View& config );
        virtual ~ProtocolCalculator() {
            delete errorProbability;
        };

        protocolCalculatorPlugins::ErrorProbability*
        getErrorProbability() const;

        protocolCalculatorPlugins::FrameLength*
        getFrameLength() const;

        protocolCalculatorPlugins::Duration*
        getDuration() const;


    private:
        void
        onMSRCreated();

        wns::logger::Logger logger;
        const wns::pyconfig::View config;

        protocolCalculatorPlugins::ErrorProbability* errorProbability;
        protocolCalculatorPlugins::FrameLength* frameLength;
        protocolCalculatorPlugins::Duration* duration;
    };
} // management
} // wifimac

#endif
