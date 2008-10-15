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

#ifndef WIFIMAC_CONVERGENCE_TXDURATIONSETTER_HPP
#define WIFIMAC_CONVERGENCE_TXDURATIONSETTER_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>

namespace wifimac { namespace convergence {

    class TxDurationProviderCommand
    {
    public:
        virtual wns::simulator::Time getDuration() const = 0;
        virtual ~TxDurationProviderCommand() {};
    };

    class TxDurationSetterCommand:
        public wns::ldk::Command,
        public TxDurationProviderCommand
    {
    public:
        TxDurationSetterCommand()
        {
            local.txDuration = 0;
        };

        struct {
            wns::simulator::Time txDuration;
        } local;

        struct {} peer;
        struct {} magic;

        wns::simulator::Time getDuration() const { return local.txDuration; }
    };


    class TxDurationSetter :
        public wns::ldk::fu::Plain<TxDurationSetter, TxDurationSetterCommand>,
        public wns::ldk::Processor<TxDurationSetter>
    {
    public:

        TxDurationSetter(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        virtual
        ~TxDurationSetter();

    private:
        /** @brief Processor Interface */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

        void onFUNCreated();

        const std::string phyUserName;
        const std::string managerName;
        wns::logger::Logger logger;

        struct Friends
        {
            wifimac::convergence::PhyUser* phyUser;
            wifimac::lowerMAC::Manager* manager;
        } friends;
    };


} // mac
} // wifimac

#endif // WIFIMAC_CONVERGENCE_TXDURATIONSETTER_HPP
