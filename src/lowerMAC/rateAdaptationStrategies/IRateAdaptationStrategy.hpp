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

#ifndef WIFIMAC_LOWERMAC_RATEADAPTATIONSTRATEGIES_IRATEADAPTATIONSTRATEGY_HPP
#define WIFIMAC_LOWERMAC_RATEADAPTATIONSTRATEGIES_IRATEADAPTATIONSTRATEGY_HPP

#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/management/PERInformationBase.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/logger/Logger.hpp>
#include <WNS/StaticFactory.hpp>
#include <WNS/PowerRatio.hpp>

namespace wifimac { namespace lowerMAC { namespace rateAdaptationStrategies {

    class IRateAdaptationStrategy
    {
    public:
        IRateAdaptationStrategy(wifimac::management::PERInformationBase*,
                                wifimac::lowerMAC::Manager*,
                                wifimac::convergence::PhyUser*,
                                wns::logger::Logger*)
            {};
        virtual ~IRateAdaptationStrategy()
            {};

        virtual wifimac::convergence::PhyMode
        getPhyMode(const wns::service::dll::UnicastAddress receiver,
                   size_t numTransmissions) = 0;

        virtual wifimac::convergence::PhyMode
        getPhyMode(const wns::service::dll::UnicastAddress receiver,
                   size_t numTransmissions,
                   const wns::Ratio lqm) = 0;
    };

    /** @brief Special creator for IRateAdaptation */
    template <typename T, typename KIND = T>
    class IRateAdaptationStrategyCreator:
        public IRateAdaptationStrategyCreator<KIND, KIND>
    {
    public:
        virtual
        KIND* create(wifimac::management::PERInformationBase* _per,
                     wifimac::lowerMAC::Manager* _manager,
                     wifimac::convergence::PhyUser* _phyUser,
                     wns::logger::Logger* _logger)
            {
                return new T(_per, _manager, _phyUser, _logger);
            }
    };

    template <typename KIND>
    class IRateAdaptationStrategyCreator<KIND, KIND>
    {
    public:
        virtual
        ~IRateAdaptationStrategyCreator() {};

        virtual KIND*
        create(wifimac::management::PERInformationBase* _per,
               wifimac::lowerMAC::Manager* _manager,
               wifimac::convergence::PhyUser* _phyUser,
               wns::logger::Logger* _logger) = 0;
    };

    typedef IRateAdaptationStrategyCreator<IRateAdaptationStrategy> RateAdaptationStrategyCreator;
    typedef wns::StaticFactory<RateAdaptationStrategyCreator> RateAdaptationStrategyFactory;
}}}

#endif
