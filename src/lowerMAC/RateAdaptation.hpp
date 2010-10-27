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
#ifndef WIFIMAC_LOWERMAC_RATEADAPTATION
#define WIFIMAC_LOWERMAC_RATEADAPTATION

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/IRateAdaptationStrategy.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/lowerMAC/ITransmissionCounter.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/management/SINRInformationBase.hpp>
#include <WIFIMAC/management/PERInformationBase.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/logger/Logger.hpp>

namespace wifimac { namespace lowerMAC {

    /**
     * @brief Setting of the rate which is used to transmit the compound
     *
     * The rate adaptation FU has, for each receiver, a
     * wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategy
     * that is used to determine the rate for the current compound.
     */
    class RateAdaptation:
        public wns::ldk::fu::Plain<RateAdaptation, wns::ldk::EmptyCommand>,
        public wns::ldk::Processor<RateAdaptation>
    {
    public:
        RateAdaptation(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
        ~RateAdaptation();

        wifimac::convergence::PhyMode
        getPhyMode(wns::service::dll::UnicastAddress receiver, size_t numTransmissions);

        wifimac::convergence::PhyMode
        getPhyMode(const wns::ldk::CompoundPtr& compound);



    private:
        void
        onFUNCreated();

        /** @brief Processor Interface */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

        const std::string phyUserName;
        const std::string managerName;
        const std::string arqName;
        const std::string sinrMIBServiceName;
        const std::string perMIBServiceName;

        /// Setting of constant rate for ACK frames
        const bool raForACKFrames;

        /**
		 * @brief Rate adaption per receiver
         */
        typedef wns::container::Registry<wns::service::dll::UnicastAddress,
                                         wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategy*,
                                         wns::container::registry::DeleteOnErase> rxRateMap;
        rxRateMap rateAdaptation;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
            wifimac::convergence::PhyUser* phyUser;
            wifimac::lowerMAC::ITransmissionCounter* arq;
        } friends;

        wifimac::management::SINRInformationBase* sinrMIB;
        wifimac::management::PERInformationBase* perMIB;

        const wns::pyconfig::View config;
        wns::logger::Logger logger;

    }; // RateAdaptation
} // lowerMAC
} // wifimac

#endif
