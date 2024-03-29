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

#include <WIFIMAC/draftn/FastLinkFeedback.hpp>

using namespace wifimac::draftn;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::FastLinkFeedback,
    wns::ldk::FunctionalUnit,
    "wifimac.draftn.FastLinkFeedback",
    wns::ldk::FUNConfigCreator);

FastLinkFeedback::FastLinkFeedback(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<FastLinkFeedback, FastLinkFeedbackCommand>(fun),
    wns::ldk::Processor<FastLinkFeedback>(),

    phyUserCommandName(config_.get<std::string>("phyUserCommandName")),
    managerName(config_.get<std::string>("managerName")),
    sinrMIBServiceName(config_.get<std::string>("sinrMIBServiceName")),
    estimatedValidity(config_.get<wns::simulator::Time>("myConfig.estimatedValidity")),
    currentPeer(),

    logger(config_.get("logger"))
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created");

    friends.manager = NULL;
    sinrMIB = NULL;
}


FastLinkFeedback::~FastLinkFeedback()
{
}

void FastLinkFeedback::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
    sinrMIB = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::draftn::SINRwithMIMOInformationBase>(sinrMIBServiceName);
}

void FastLinkFeedback::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    if(getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), this))
    {
        if(getCommand(compound->getCommandPool())->peer.isRequest)
        {
            currentPeer = friends.manager->getTransmitterAddress(compound->getCommandPool());
            wns::Ratio sinr = getFUN()->getCommandReader(phyUserCommandName)->
                readCommand<wifimac::convergence::PhyUserCommand>(compound->getCommandPool())->getCIRwithoutMIMO();
            sinrMIB->putMeasurement(currentPeer, sinr, estimatedValidity);

            MESSAGE_SINGLE(NORMAL, this->logger, "Request from " << currentPeer << ", measured SINR " << sinr);
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "Reply from " << friends.manager->getTransmitterAddress(compound->getCommandPool()) << ", with SINR " << getCommand(compound->getCommandPool())->peer.cqi);
            sinrMIB->putPeerSINR(friends.manager->getTransmitterAddress(compound->getCommandPool()),
                                 getCommand(compound->getCommandPool())->peer.cqi,
                                 estimatedValidity);
            for(std::vector< std::vector<wns::Ratio> >::iterator it = getCommand(compound->getCommandPool())->peer.mimoFactors.begin();
                it != getCommand(compound->getCommandPool())->peer.mimoFactors.end();
                ++it)
            {
                sinrMIB->putPeerFactor(friends.manager->getTransmitterAddress(compound->getCommandPool()),
                                       *it);
#ifndef WNS_NO_LOGGING
                MESSAGE_BEGIN(NORMAL, logger, m, "Contains peer factors");
                for(std::vector<wns::Ratio>::iterator itFactors = it->begin();
                    itFactors != it->end();
                    ++itFactors)
                {
                    m << " " << *itFactors;
                }
                MESSAGE_END();
#endif
            }
        }
    }
    else
    {
        currentPeer = wns::service::dll::UnicastAddress();
    }
}

void FastLinkFeedback::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    if(currentPeer == friends.manager->getReceiverAddress(compound->getCommandPool()))
    {
        if(sinrMIB->knowsMeasuredSINR(currentPeer))
        {
            FastLinkFeedbackCommand* flfc = activateCommand(compound->getCommandPool());
            flfc->peer.isRequest = false;
            flfc->peer.cqi = sinrMIB->getMeasuredSINR(currentPeer);

            for(unsigned int numSS = 1; numSS <= friends.manager->getNumAntennas(); ++numSS)
            {
                if(sinrMIB->knowsMeasuredFactor(currentPeer, numSS))
                {
                    flfc->peer.mimoFactors.push_back(sinrMIB->getMeasuredFactor(currentPeer, numSS));
                }
            }

#ifndef WNS_NO_LOGGING
            MESSAGE_BEGIN(NORMAL, this->logger, m, "Outgoing frame to requester " << currentPeer);
            m << ", piggyback measured SINR " << flfc->peer.cqi;
            m << ", pF:";
            for(unsigned int numSS = 1; numSS <= friends.manager->getNumAntennas(); ++numSS)
            {
                m << "(";
                if(sinrMIB->knowsMeasuredFactor(currentPeer, numSS))
                {
                    std::vector<wns::Ratio> pF = sinrMIB->getMeasuredFactor(currentPeer, numSS);
                    for(std::vector<wns::Ratio>::iterator it = pF.begin();
                        it != pF.end();
                        ++it)
                    {
                        m << " " << *it;
                    }
                }
                m << ")";
            }
            MESSAGE_END();
#endif
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "Outgoing frame to requester " << currentPeer << ", but SINR is not known!");
        }
        currentPeer = wns::service::dll::UnicastAddress();
        return;
    }

    if(friends.manager->getReplyTimeout(compound->getCommandPool()) > 0)
    {
        FastLinkFeedbackCommand* flfc = activateCommand(compound->getCommandPool());
        flfc->peer.isRequest = true;
        MESSAGE_SINGLE(NORMAL, this->logger, "Outgoing frame to " << friends.manager->getReceiverAddress(compound->getCommandPool()) << ", use as request");
        return;
    }
}

