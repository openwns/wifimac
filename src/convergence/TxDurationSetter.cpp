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

#include <WIFIMAC/convergence/TxDurationSetter.hpp>

using namespace wifimac::convergence;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::convergence::TxDurationSetter,
    wns::ldk::FunctionalUnit,
    "wifimac.convergence.TxDurationSetter",
    wns::ldk::FUNConfigCreator);

TxDurationSetter::TxDurationSetter(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<TxDurationSetter, TxDurationSetterCommand>(fun),

    phyUserName(config_.get<std::string>("phyUserName")),
    managerName(config_.get<std::string>("managerName")),

    logger(config_.get("logger"))
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created");

    friends.phyUser = NULL;
    friends.manager = NULL;
}


TxDurationSetter::~TxDurationSetter()
{
}

void TxDurationSetter::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.phyUser = getFUN()->findFriend<wifimac::convergence::PhyUser*>(phyUserName);
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
}

void
TxDurationSetter::processIncoming(const wns::ldk::CompoundPtr& /*compound*/)
{

}

void
TxDurationSetter::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    TxDurationSetterCommand* command = activateCommand(compound->getCommandPool());

    // calculate tx duration
    wns::simulator::Time frameTxDuration = 0;
    if(friends.manager->getFrameType(compound->getCommandPool()) == PREAMBLE)
    {
        command->local.txDuration = friends.phyUser->getPreambleDuration();

        MESSAGE_BEGIN(NORMAL, this->logger, m, "Preamble");
        m << ": duration " << command->local.txDuration;
        MESSAGE_END();
    }
    else
    {
        command->local.txDuration = friends.phyUser->getPSDUDuration(compound);

        MESSAGE_BEGIN(NORMAL, this->logger, m, "Command");
        m << " start " << wns::simulator::getEventScheduler()->getTime();
        m << " stop " << wns::simulator::getEventScheduler()->getTime() + frameTxDuration;
        MESSAGE_END();

    }
}

