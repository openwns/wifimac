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

#include <WIFIMAC/draftn/DeAggregation.hpp>

#include <DLL/Layer2.hpp>

#include <WNS/ldk/concatenation/Concatenation.hpp>


using namespace wifimac::draftn;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::DeAggregation,
    wns::ldk::FunctionalUnit,
    "wifimac.draftn.DeAggregation",
    wns::ldk::FUNConfigCreator);

DeAggregation::DeAggregation(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<DeAggregation, DeAggregationCommand>(fun),

    managerName(config_.get<std::string>("managerName")),
    protocolCalculatorName(config_.get<std::string>("protocolCalculatorName")),
    txStartEndName(config_.get<std::string>("phyUserName")),
    aggregationCommandName(config_.get<std::string>("aggregationCommandName")),

    txQueue(),
    currentTxCompound(),
    currentRxContainer(),
    doSignalTxStart(false),
    numEntries(0),

    logger(config_.get("logger"))
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created");

    protocolCalculator = NULL;
    friends.manager = NULL;
}


DeAggregation::~DeAggregation()
{
}

void DeAggregation::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");
    protocolCalculator = getFUN()->getLayer<dll::ILayer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);

    // Observe txStartEnd
    this->wns::Observer<wifimac::convergence::ITxStartEnd>::startObserving(getFUN()->findFriend<wifimac::convergence::TxStartEndNotification*>(txStartEndName));
}

void
DeAggregation::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    DeAggregationCommand* command = getCommand(compound->getCommandPool());

    if(command->peer.singleFragment)
    {
        // no aggregation at all
        getDeliverer()->getAcceptor(compound)->onData(compound);
        return;
    }

    if(friends.manager->getFrameType(compound->getCommandPool()) == PREAMBLE)
    {
        // new aggregation
        if(this->currentRxContainer)
        {
            // capture effect: we receive a preamble of a different compound
            // before the current container is received completely.
            MESSAGE_SINGLE(NORMAL, this->logger, "Received aggregation preamble, during aggregation -> deliver old with " << this->numEntries << " entries");
            getDeliverer()->getAcceptor(this->currentRxContainer)->onData(this->currentRxContainer);
            this->currentRxContainer = wns::ldk::CompoundPtr();
            this->numEntries = 0;
            this->cancelTimeout();
        }

        // generate container from preamble
        this->currentRxContainer = compound->copy();
        friends.manager->setFrameType(this->currentRxContainer->getCommandPool(), DATA);

        // clear compound container
        getFUN()->getCommandReader(aggregationCommandName)->
            readCommand<wns::ldk::concatenation::ConcatenationCommand>(this->currentRxContainer->getCommandPool())->peer.compounds.clear();

        MESSAGE_SINGLE(NORMAL, this->logger, "Received aggregation preamble, create container");

        // set timeout for the delivery of the compounds
        this->setTimeout(friends.manager->getFrameExchangeDuration(compound->getCommandPool()) + 10e-9);

        // deliver preamble
        getDeliverer()->getAcceptor(compound)->onData(compound);
        return;
    }

    // append current compound
    wns::ldk::concatenation::ConcatenationCommand* aggCommand = getFUN()->getCommandReader(aggregationCommandName)->
        readCommand<wns::ldk::concatenation::ConcatenationCommand>(this->currentRxContainer->getCommandPool());
    aggCommand->peer.compounds.push_back(compound);

    MESSAGE_SINGLE(NORMAL, this->logger, "Received aggregation fragment, append");
    this->numEntries++;

    if(command->peer.finalFragment)
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "Received last aggregation fragment, deliver container with " << this->numEntries << " entries");
        // final fragment -> deliver complete aggregated compound
        wns::ldk::CompoundPtr it = this->currentRxContainer->copy();
        this->currentRxContainer = wns::ldk::CompoundPtr();
        this->numEntries = 0;
        this->cancelTimeout();

        getDeliverer()->getAcceptor(it)->onData(it);
    }
}

void DeAggregation::onTimeout()
{
    if(this->numEntries > 0)
    {
        assure(this->currentRxContainer, "No current container");
        MESSAGE_SINGLE(NORMAL, this->logger, "Timeout for last aggregation fragment, deliver container with " << this->numEntries << " entries");
        getDeliverer()->getAcceptor(this->currentRxContainer)->onData(this->currentRxContainer);
        this->currentRxContainer = wns::ldk::CompoundPtr();
        this->numEntries = 0;
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "Timeout for aggregation, no entries successfully received");
        this->currentRxContainer = wns::ldk::CompoundPtr();
    }
}


void
DeAggregation::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    assure(this->currentTxCompound == wns::ldk::CompoundPtr(), "processOutgoing, but currentTxCompound is not free");
    assure(this->txQueue.empty(), "processOutgoing, but txQueue is not empty");

    // calculate tx duration
    DeAggregationCommand* command = activateCommand(compound->getCommandPool());
    wifimac::convergence::PhyMode phyMode = friends.manager->getPhyMode(compound->getCommandPool());
    wns::simulator::Time preambleTxDuration = protocolCalculator->getDuration()->preamble(phyMode);

    if(friends.manager->getFrameType(compound->getCommandPool()) == PREAMBLE)
    {
        command->local.txDuration = preambleTxDuration;
        if(getFUN()->getCommandReader(aggregationCommandName)->commandIsActivated(compound->getCommandPool()))
        {
            // preamble of aggregation command
            command->peer.singleFragment = false;
            command->peer.finalFragment = false;
        }
        else
        {
            command->peer.singleFragment = true;
        }
        this->txQueue.push_back(compound);
        return;
    }

    this->currentTxCompound = compound;
    this->doSignalTxStart = true;

    wns::simulator::Time frameTxDuration = protocolCalculator->getDuration()->MPDU_PPDU(compound->getLengthInBits(), phyMode) - preambleTxDuration;

    if(not getFUN()->getCommandReader(aggregationCommandName)->commandIsActivated(compound->getCommandPool()))
    {
        // not an aggregation container command, schedule transmission for now
        command->local.txDuration = frameTxDuration;
        command->peer.singleFragment = true;
        this->txQueue.push_back(compound);
        return;
    }

    wns::simulator::Time nav = friends.manager->getFrameExchangeDuration(compound->getCommandPool());
    assure(frameTxDuration > 0, "Cannot transmit a frame with duration <= 0");

    MESSAGE_SINGLE(NORMAL, logger, "Outgoing aggregated compound with frame duration " << frameTxDuration);

    // split aggregation container into single compounds
    std::vector<Bit> sduSizes = std::vector<Bit>();
    Bit completeSize = 0;

    // iterate over the compounds to get the complete and single sizes
    wns::ldk::concatenation::ConcatenationCommand* aggCommand = getFUN()->getCommandReader(aggregationCommandName)->
        readCommand<wns::ldk::concatenation::ConcatenationCommand>(compound->getCommandPool());
    for (std::vector<wns::ldk::CompoundPtr>::iterator it = aggCommand->peer.compounds.begin();
         it != aggCommand->peer.compounds.end();
         ++it)
    {
        completeSize += (*it)->getLengthInBits();
        sduSizes.push_back((*it)->getLengthInBits());
    }

    MESSAGE_SINGLE(NORMAL, logger, aggCommand->peer.compounds.size() << " entries in aggregation container, complete size " << completeSize);

    // iterate over the compounds and queue transmissions
    wns::simulator::Time nextStart = 0.0;
    std::vector<Bit>::iterator itS = sduSizes.begin();
    for (std::vector<wns::ldk::CompoundPtr>::iterator itC = aggCommand->peer.compounds.begin();
         itC != aggCommand->peer.compounds.end();
         ++itC, ++itS)
    {
        // set duration field to end of transmission + former nav
        wns::simulator::Time currentDuration = static_cast<double>(*itS)/static_cast<double>(completeSize) * frameTxDuration;
        friends.manager->setFrameExchangeDuration((*itC)->getCommandPool(), (frameTxDuration - nextStart - currentDuration) + nav);

        // all fragments must have the same phyMode
        friends.manager->setPhyMode((*itC)->getCommandPool(), friends.manager->getPhyMode(compound->getCommandPool()));

        // schedule fragment transmission
        DeAggregationCommand* currentCommand = activateCommand((*itC)->getCommandPool());
        currentCommand->local.txDuration = currentDuration;

        nextStart = nextStart + currentDuration;

        currentCommand->peer.singleFragment = false;
        // mark last compound
        currentCommand->peer.finalFragment = (itC+1 == aggCommand->peer.compounds.end());

        // next fragment, queue for transmission
        this->txQueue.push_back(*itC);

        MESSAGE_BEGIN(NORMAL, logger, m, " * size " << (*itS));
        m << ", fraction " << static_cast<double>(*itS)/static_cast<double>(completeSize);
        m << "-> delay " << nextStart - currentDuration;
        m << " duration " << currentDuration;
        m << " final " << currentCommand->peer.finalFragment;
        MESSAGE_END();
    }
}

const wns::ldk::CompoundPtr
DeAggregation::hasSomethingToSend() const
{
    if(txQueue.empty())
    {
        return(wns::ldk::CompoundPtr());
    }
    else
    {
        return(*(txQueue.begin()));
    }
}

wns::ldk::CompoundPtr
DeAggregation::getSomethingToSend()
{
    wns::ldk::CompoundPtr send = *(this->txQueue.begin());
    this->txQueue.pop_front();
    return(send);
}

bool
DeAggregation::hasCapacity() const
{
    return(this->currentTxCompound == wns::ldk::CompoundPtr());
}

void
DeAggregation::calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const
{

    getFUN()->getProxy()->calculateSizes(commandPool, commandPoolSize, dataSize, this);
}

void
DeAggregation::onTxStart(const wns::ldk::CompoundPtr& /*compound*/)
{
    if(this->doSignalTxStart)
    {
        this->doSignalTxStart = false;
        this->wns::Subject<ITxStartEnd>::forEachObserver(OnTxStartEnd(this->currentTxCompound, wifimac::convergence::start));
    }
}

void
DeAggregation::onTxEnd(const wns::ldk::CompoundPtr& /*compound*/)
{
    if((this->txQueue.empty()) and (this->currentTxCompound != wns::ldk::CompoundPtr()))
    {
        // signal tx end of complete frame
        this->wns::Subject<ITxStartEnd>::forEachObserver(OnTxStartEnd(this->currentTxCompound, wifimac::convergence::end));
        this->currentTxCompound = wns::ldk::CompoundPtr();
    }
}
