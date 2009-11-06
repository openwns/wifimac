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

#include <WIFIMAC/draftn/BlockUntilReply.hpp>

using namespace wifimac::draftn;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::BlockUntilReply,
    wns::ldk::FunctionalUnit,
    "wifimac.draftn.BlockUntilReply",
    wns::ldk::FUNConfigCreator);

BlockUntilReply::BlockUntilReply(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config):
    wns::ldk::fu::Plain<BlockUntilReply, wns::ldk::EmptyCommand>(fun),

    sifsDuration(config.get<wns::simulator::Time>("myConfig.sifsDuration")),
    managerName(config.get<std::string>("managerName")),
    txStartEndName(config.get<std::string>("txStartEndName")),
    rxStartEndName(config.get<std::string>("rxStartEndName")),
    blocked(false),
    txStatus(finished),
    blockedBy(),
    logger(config.get("logger"))
{

}

BlockUntilReply::~BlockUntilReply()
{

}

void BlockUntilReply::onFUNCreated()
{
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);

    this->wns::Observer<wifimac::convergence::ITxStartEnd>::startObserving
        (getFUN()->findFriend<wifimac::convergence::TxStartEndNotification*>(txStartEndName));
    this->wns::Observer<wifimac::convergence::IRxStartEnd>::startObserving
        (getFUN()->findFriend<wifimac::convergence::RxStartEndNotification*>(rxStartEndName));
}

void BlockUntilReply::doSendData(const wns::ldk::CompoundPtr& compound)
{
    if(friends.manager->getFrameType(compound->getCommandPool()) == ACK)
    {
        getConnector()->getAcceptor(compound)->sendData(compound);
        return;
    }
    this->blocked = true;
    this->txStatus = waiting;
    this->blockedBy = compound->getBirthmark();
    MESSAGE_SINGLE(NORMAL, logger, "Ougoing data -> block");

    // pass through
    getConnector()->getAcceptor(compound)->sendData(compound);
}

void BlockUntilReply::doOnData(const wns::ldk::CompoundPtr& compound)
{
    if(this->blocked and (this->txStatus == finished))
    {
        this->blocked = false;

        if(hasTimeoutSet())
        {
            cancelTimeout();
        }

        MESSAGE_SINGLE(NORMAL, logger, "Received data -> remove block");
    }
    // pass through
    getDeliverer()->getAcceptor(compound)->onData(compound);
}

bool BlockUntilReply::doIsAccepting(const wns::ldk::CompoundPtr& compound) const
{
    if(friends.manager->getFrameType(compound->getCommandPool()) == ACK)
    {
        return (this->txStatus != transmitting) and (getConnector()->hasAcceptor(compound));
    }

    if(this->blocked)
    {
        return(false);
    }
    else
    {
        return getConnector()->hasAcceptor(compound);
    }
}

void BlockUntilReply::doWakeup()
{
    if(not this->blocked)
    {
        getReceptor()->wakeup();
    }
}

void BlockUntilReply::onTxStart(const wns::ldk::CompoundPtr& compound)
{
    if(compound->getBirthmark() == this->blockedBy)
    {
        assure(this->txStatus == waiting, "onTxStart, but status is not waiting");
        MESSAGE_SINGLE(NORMAL, logger, "Start of frame transmission");
        this->txStatus = transmitting;
    }

}

void BlockUntilReply::onTxEnd(const wns::ldk::CompoundPtr& compound)
{
    if(compound->getBirthmark() != this->blockedBy)
    {
        return;
    }

    assure(this->txStatus == transmitting, "onTxEnd, but status is not transmitting");
    this->txStatus = finished;
    wns::simulator::Time timeout = friends.manager->getReplyTimeout(compound->getCommandPool());
    if(timeout > 0.0)
    {
        // Timeout for reception
        setTimeout(timeout);
        MESSAGE_SINGLE(NORMAL, logger, "End frame transmission, requires reply -> wait for " << timeout);
    }
    else
    {
        if(this->blocked)
        {
            this->blocked = false;
            MESSAGE_SINGLE(NORMAL, logger, "End frame transmission, remove block");
            getReceptor()->wakeup();
        }
    }
}

void BlockUntilReply::onTimeout()
{
    assure(this->blocked, "timeout, but not blocked");
    assure(this->txStatus == finished, "tx is not done, but timeout");

    this->blocked = false;
    MESSAGE_SINGLE(NORMAL, logger, "timeout -> remove block");
    getReceptor()->wakeup();
}

void BlockUntilReply::onRxStart(wns::simulator::Time /*expRxTime*/)
{
    if(this->blocked and (this->txStatus == finished))
    {
        MESSAGE_SINGLE(NORMAL, logger, "onRxStart during blocked, wait for frame");
        if(hasTimeoutSet())
        {
            cancelTimeout();
        }
    }
}


void BlockUntilReply::onRxEnd()
{
    if(this->blocked and (this->txStatus == finished))
    {
        setTimeout(10e-9);
        MESSAGE_SINGLE(NORMAL, logger, "onRxEnd during blocked, wait for frame");
    }
}

void BlockUntilReply::onRxError()
{
    if(this->blocked and (this->txStatus == finished))
    {
        MESSAGE_SINGLE(NORMAL, logger, "onRxError during blocked, timeout");
        onTimeout();
    }
}
