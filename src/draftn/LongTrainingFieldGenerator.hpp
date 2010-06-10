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

#ifndef WIFIMAC_DRAFTN_PREAMBLEGENERATOR_HPP
#define WIFIMAC_DRAFTN_PREAMBLEGENERATOR_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/management/ProtocolCalculator.hpp>
#include <WIFIMAC/draftn/DeAggregation.hpp>
#include <WIFIMAC/draftn/SINRwithMIMOInformationBase.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>

namespace wifimac { namespace draftn {

    class LongTrainingFieldGeneratorCommand:
        public wns::ldk::Command
    {
    public:
        struct {} local;
        struct {
            // id of the upcoming frame to which the ht-ltf belongs to
            wns::Birthmark frameId;
        } peer;
        struct {} magic;

        wns::Birthmark getFrameId() const
        {
            return(this->peer.frameId);
        }
    };

    /**
     * @brief Postfixes the preamble with a HT-LTFs
     */
    class LongTrainingFieldGenerator:
        public wns::ldk::fu::Plain<LongTrainingFieldGenerator, LongTrainingFieldGeneratorCommand>,
        public wns::ldk::Delayed<LongTrainingFieldGenerator>
    {
    public:
        LongTrainingFieldGenerator(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        virtual
        ~LongTrainingFieldGenerator();

    private:
        /** @brief Delayed Interface */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);
        bool hasCapacity() const;
        const wns::ldk::CompoundPtr hasSomethingToSend() const;
        wns::ldk::CompoundPtr getSomethingToSend();

        void onFUNCreated();

        /**
		 * @brief SDU and PCI size calculation for preambles -> no size!
		 */
        void calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const;

        const std::string phyUserName;
        const std::string protocolCalculatorName;
        const std::string managerName;
        const std::string txDurationSetterName;
        const std::string sinrMIBServiceName;

        const wns::simulator::Time ltfDuration;
        const bool reducePreambleByDuration;

        int numLTFsToSend;

        wns::logger::Logger logger;

        wns::ldk::CompoundPtr pendingCompound;
        wifimac::management::ProtocolCalculator* protocolCalculator;

        struct Friends
        {
            wifimac::convergence::PhyUser* phyUser;
            wifimac::lowerMAC::Manager* manager;
            wifimac::draftn::DeAggregation* txDuration;
        } friends;

        wifimac::draftn::SINRwithMIMOInformationBase* sinrMIB;
    };
} // mac
} // wifimac

#endif // WIFIMAC_DRAFTN_PREAMBLEGENERATOR_HPP
