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

#ifndef WIFIMAC_LOWERMAC_RATEADAPTATIONSTRATEGIES_CONSTANT_HPP
#define WIFIMAC_LOWERMAC_RATEADAPTATIONSTRATEGIES_CONSTANT_HPP

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/IRateAdaptationStrategy.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/service/dll/Address.hpp>

namespace wifimac { namespace lowerMAC { namespace rateAdaptationStrategies {

    /**
     * @brief Constant Rate Adaption: Always use configured MCS
     */
    class Constant :
        public IRateAdaptationStrategy
    {
    public:
        Constant(
            const wns::pyconfig::View& config_,
            wns::service::dll::UnicastAddress receiver_,
            wifimac::management::PERInformationBase*,
            wifimac::management::SINRInformationBase*,
            wifimac::lowerMAC::Manager*,
            wifimac::convergence::PhyUser*,
            wns::logger::Logger*);


        wifimac::convergence::PhyMode
        getPhyMode(size_t numTransmissions) const;

        wifimac::convergence::PhyMode
        getPhyMode(size_t numTransmissions,
                   const wns::Ratio lqm) const;

        void
        setCurrentPhyMode(wifimac::convergence::PhyMode pm);   

    private:
        const wifimac::convergence::PhyMode myPM;

        struct Friends
        {
            wifimac::convergence::PhyUser* phyUser;
        } friends;
    };
}}}
#endif // #ifndef WIFIMAC_LOWERMAC_RATEADAPTIONSTRATEGIES_CONSTANT_HPP
