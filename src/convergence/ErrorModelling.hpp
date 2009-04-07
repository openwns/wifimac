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

#ifndef WIFIMAC_CONVERGENCE_ERRORMODELLING
#define WIFIMAC_CONVERGENCE_ERRORMODELLING

#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/management/ProtocolCalculator.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>
#include <WNS/ldk/ErrorRateProviderInterface.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/logger/Logger.hpp>

namespace wifimac { namespace convergence {

    /**
	 * @brief ErrorModellingCommand, which stores the per/cir
	 */
    class ErrorModellingCommand:
        public wns::ldk::Command,
        public wns::ldk::ErrorRateProviderInterface
    {
    public:
        ErrorModellingCommand()
            {
                local.per = 1;
                local.destructorCalled = NULL;
                local.cir.set_dB(0);
            }

        ~ErrorModellingCommand()
            {
                if(local.destructorCalled != NULL)
                    *local.destructorCalled = true;
            }
        virtual double getErrorRate() const
            {
                return local.per;
            }

        struct {
            double per;
            long *destructorCalled;
            wns::Ratio cir;
        } local;
        struct {} peer;
        struct {} magic;
    };

    /**
	 * @brief ErrorModelling implementation of the FU.
	 *
	 * It maps the Carrier Interference Ratio (CIR) for an MCS to the Packet
	 * Error Rate (PER).
	 */
    class ErrorModelling:
        public wns::ldk::fu::Plain<ErrorModelling, ErrorModellingCommand>,
        public wns::ldk::Processor<ErrorModelling>
    {
    public:
        // FUNConfigCreator interface realisation
        ErrorModelling(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        /**
         * @brief Processor Interface Implementation
         */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

        void onFUNCreated();

    private:
        wns::pyconfig::View config;
        wns::logger::Logger logger;
        const std::string phyUserCommandName;
        const std::string managerCommandName;
        const std::string protocolCalculatorName;

        wifimac::management::ProtocolCalculator* pc;

    }; // ErrorModelling
} // convergence
} // wifimac

#endif
