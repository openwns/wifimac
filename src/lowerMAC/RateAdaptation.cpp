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

#include <WIFIMAC/lowerMAC/RateAdaptation.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::RateAdaptation,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.RateAdaptation",
    wns::ldk::FUNConfigCreator);

RateAdaptation::RateAdaptation(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<RateAdaptation, wns::ldk::EmptyCommand>(fun),
    wns::ldk::Processor<RateAdaptation>(),

    phyUserName(config_.get<std::string>("phyUserName")),
    managerName(config_.get<std::string>("managerName")),
    sinrMIBServiceName(config_.get<std::string>("sinrMIBServiceName")),
    perMIBServiceName(config_.get<std::string>("perMIBServiceName")),
    raForACKFrames(config_.get<bool>("raForACKFrames")),
    ackFramesRateId(config_.get<int>("ackFramesRateId")),

    logger(config_.get("logger"))
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created");

    friends.phyUser = NULL;
    friends.manager = NULL;

    raCreator = rateAdaptationStrategies::RateAdaptationStrategyFactory::creator(config_.get<std::string>("raStrategy"));
}


RateAdaptation::~RateAdaptation()
{
}

void RateAdaptation::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.phyUser = getFUN()->findFriend<wifimac::convergence::PhyUser*>(phyUserName);
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);

    sinrMIB = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::management::SINRInformationBase>(sinrMIBServiceName);
    perMIB = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::management::PERInformationBase>(perMIBServiceName);

    ackFramesRate = friends.phyUser->getPhyModeProvider()->getPhyMode(ackFramesRateId);
}

void RateAdaptation::processIncoming(const wns::ldk::CompoundPtr& /*compound*/)
{
    // we do exactly nothing with incoming compounds
}

void RateAdaptation::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    if((not raForACKFrames) and (friends.manager->getFrameType(compound->getCommandPool()) == ACK))
    {
        friends.manager->setPhyMode(compound->getCommandPool(), this->ackFramesRate);

        MESSAGE_BEGIN(NORMAL, logger, m, "Send ACK frame to ");
        m << " rx: " << friends.manager->getReceiverAddress(compound->getCommandPool());
        m << " with " << friends.manager->getPhyMode(compound->getCommandPool());
        MESSAGE_END();
    }
    else
    {
        wns::service::dll::UnicastAddress receiver = friends.manager->getReceiverAddress(compound->getCommandPool());
        friends.manager->setPhyMode(compound->getCommandPool(), this->getPhyMode(receiver));

        MESSAGE_BEGIN(NORMAL, logger, m, "Send data frame to ");
        m << " rx: " << friends.manager->getReceiverAddress(compound->getCommandPool());
        m << " with " << friends.manager->getPhyMode(compound->getCommandPool());
        MESSAGE_END();
    }
}

wifimac::convergence::PhyMode
RateAdaptation::getPhyMode(wns::service::dll::UnicastAddress receiver)
{
    wifimac::convergence::PhyMode pm;

    if(not rateAdaptation.knows(receiver) )
    {
        MESSAGE_SINGLE(NORMAL, logger, "No matching RA found for receiver " << receiver << ", create new");
        rateAdaptation.insert(receiver,
                              raCreator->create(perMIB,
                                                friends.manager,
                                                friends.phyUser,
                                                &logger));
    }

    if(sinrMIB->knowsPeerSINR(receiver))
    {
        pm = rateAdaptation.find(receiver)->getPhyMode(receiver, sinrMIB->getAveragePeerSINR(receiver));
    }
    else
    {
        pm = rateAdaptation.find(receiver)->getPhyMode(receiver);
    }
    return(pm);
}
