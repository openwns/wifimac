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
#ifndef WIFIMAC_DRAFTN_FASTLINKFEEDBACK
#define WIFIMAC_DRAFTN_FASTLINKFEEDBACK

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/draftn/SINRwithMIMOInformationBase.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/logger/Logger.hpp>

namespace wifimac { namespace draftn {

    class FastLinkFeedbackCommand:
        public wns::ldk::Command
    {
    public:
        struct {} local;
        struct {
            bool isRequest;
            wns::Ratio cqi;
            std::vector< std::vector<wns::Ratio> > mimoFactors;
        } peer;
        struct {} magic;
    };

    /**
     * @brief Fast link-feedback piggybacked on frame exchanged
     */
    class FastLinkFeedback:
        public wns::ldk::fu::Plain<FastLinkFeedback, FastLinkFeedbackCommand>,
        public wns::ldk::Processor<FastLinkFeedback>
    {
    public:
        FastLinkFeedback(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
        ~FastLinkFeedback();

    private:
        void
        onFUNCreated();

        /** @brief Processor Interface */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

        const std::string managerName;
        const std::string phyUserCommandName;
        const std::string sinrMIBServiceName;
        const wns::simulator::Time estimatedValidity;

        wns::service::dll::UnicastAddress currentPeer;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;

        wifimac::draftn::SINRwithMIMOInformationBase* sinrMIB;

        wns::logger::Logger logger;

    }; // FastLinkFeedback
} // draftn
} // wifimac

#endif
