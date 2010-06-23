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

#include <WIFIMAC/draftn/LongTrainingFieldGenerator.hpp>
#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>
#include <DLL/Layer2.hpp>

using namespace wifimac::draftn;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::LongTrainingFieldGenerator,
    wns::ldk::FunctionalUnit,
    "wifimac.draftn.LongTrainingFieldGenerator",
    wns::ldk::FUNConfigCreator);

LongTrainingFieldGenerator::LongTrainingFieldGenerator(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<LongTrainingFieldGenerator, LongTrainingFieldGeneratorCommand>(fun),
    phyUserName(config_.get<std::string>("phyUserName")),
    protocolCalculatorName(config_.get<std::string>("protocolCalculatorName")),
    managerName(config_.get<std::string>("managerName")),
    txDurationSetterName(config_.get<std::string>("txDurationSetterName")),
    sinrMIBServiceName(config_.get<std::string>("sinrMIBServiceName")),
    ltfDuration(config_.get<wns::simulator::Time>("myConfig.ltfDuration")),
    reducePreambleByDuration(config_.get<bool>("myConfig.reducePreambleByDuration")),
    numLTFsToSend(0),
    logger(config_.get("logger")),
    pendingCompound()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created");
    friends.manager = NULL;
    friends.phyUser = NULL;
    friends.txDuration = NULL;
    protocolCalculator = NULL;
}

LongTrainingFieldGenerator::~LongTrainingFieldGenerator()
{

}

void
LongTrainingFieldGenerator::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.phyUser = getFUN()->findFriend<wifimac::convergence::PhyUser*>(phyUserName);
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
    friends.txDuration = getFUN()->findFriend<wifimac::draftn::DeAggregation*>(txDurationSetterName);
    protocolCalculator = getFUN()->getLayer<dll::ILayer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);

    sinrMIB = getFUN()->getLayer<dll::ILayer2*>()->getManagementService<wifimac::draftn::SINRwithMIMOInformationBase>(sinrMIBServiceName);
}

void
LongTrainingFieldGenerator::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    if(friends.manager->getFrameType(compound->getCommandPool()) == PREAMBLE)
    {

        std::vector<wns::Ratio> postSINRFactor = friends.phyUser->getCommand(compound->getCommandPool())->local.postSINRFactor;
        sinrMIB->putFactorMeasurement(friends.manager->getTransmitterAddress(compound->getCommandPool()),
                                      postSINRFactor);

        MESSAGE_BEGIN(NORMAL, logger, m, "Received Preamble/LTF with ");
        m << postSINRFactor.size() << " streams. PostSINRFactor:";
        for(std::vector<wns::Ratio>::iterator it = postSINRFactor.begin();
            it != postSINRFactor.end();
            ++it)
        {
            m << " " << *it;
        }
        MESSAGE_END();

        if(friends.manager->getPhyMode(compound->getCommandPool()).getNumberOfSpatialStreams() > 1)
        {
            // drop LTF
            MESSAGE_BEGIN(NORMAL, logger, m, "Dropping incoming LTF with");
            m << friends.manager->getPhyMode(compound->getCommandPool()).getNumberOfSpatialStreams();
            m << "spatial streams";
            MESSAGE_END();
            return;
        }
    }

    // deliver frame
    getDeliverer()->getAcceptor(compound)->onData(compound);
}

void
LongTrainingFieldGenerator::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    this->pendingCompound = compound;

    if(friends.manager->getFrameType(compound->getCommandPool()) == PREAMBLE)
    {
        wns::service::dll::UnicastAddress receiver = friends.manager->getReceiverAddress(compound->getCommandPool());
        unsigned int numTx = friends.manager->getNumAntennas();
        unsigned int numRx = 1;
        if(wifimac::management::TheVCIBService::Instance().getVCIB()->knows(receiver, "numAntennas"))
        {
            numRx = wifimac::management::TheVCIBService::Instance().getVCIB()->get<int>(receiver, "numAntennas");
        }
        unsigned int maxNumSS = (numTx < numRx) ? numTx : numRx;
        numLTFsToSend = maxNumSS - 1;
        if(numLTFsToSend > 0)
        {
            if(reducePreambleByDuration)
            {
                friends.txDuration->getCommand(compound->getCommandPool())->local.txDuration -= numLTFsToSend*ltfDuration;
            }
            friends.manager->setFrameExchangeDuration(compound->getCommandPool(),
                                                      friends.manager->getFrameExchangeDuration(compound->getCommandPool())+numLTFsToSend*ltfDuration);

            MESSAGE_BEGIN(NORMAL, logger, m, "Outgoing preamble to " << receiver);
            m << " maxNumSS: " << maxNumSS;
            if(reducePreambleByDuration)
            {
                m << " reduce txDuration to " << friends.txDuration->getCommand(compound->getCommandPool())->local.txDuration << " and";
            }
            m << " insert " << numLTFsToSend << " LTFs";
            MESSAGE_END();
        }
    }

}

const wns::ldk::CompoundPtr
LongTrainingFieldGenerator::hasSomethingToSend() const
{
    return(this->pendingCompound);
}

wns::ldk::CompoundPtr
LongTrainingFieldGenerator::getSomethingToSend()
{
    if(this->pendingCompound)
    {
        wns::ldk::CompoundPtr myFrame = this->pendingCompound;
        if(numLTFsToSend == 0)
        {
            this->pendingCompound = wns::ldk::CompoundPtr();
            MESSAGE_SINGLE(NORMAL, logger, "Sending psdu");
        }
        else
        {
            wns::ldk::CompoundPtr ltf = this->pendingCompound->copy();
            friends.txDuration->getCommand(ltf->getCommandPool())->local.txDuration = ltfDuration;
            wifimac::convergence::PhyMode pm = friends.manager->getPhyMode(ltf->getCommandPool());
            pm.setUniformMCS(pm.getSpatialStreams()[0], pm.getNumberOfSpatialStreams()+1);
            friends.manager->setPhyMode(ltf->getCommandPool(), pm);
            friends.manager->setFrameExchangeDuration(ltf->getCommandPool(),
                                                      friends.manager->getFrameExchangeDuration(ltf->getCommandPool())-ltfDuration);


            this->pendingCompound = ltf;

            --numLTFsToSend;
            MESSAGE_SINGLE(NORMAL, logger, "Prepared LTF with " << pm.getNumberOfSpatialStreams() << " spatial streams, to be send after current pending compound");
        }

        return(myFrame);
    }
}

bool
LongTrainingFieldGenerator::hasCapacity() const
{
    return(this->pendingCompound == wns::ldk::CompoundPtr() and (not friends.phyUser->isTransmitting()));
}

void
LongTrainingFieldGenerator::calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const
{
    if(friends.manager->getFrameType(commandPool) == PREAMBLE)
    {
        commandPoolSize = 0;
        dataSize = 0;
    }
    else
    {
        getFUN()->getProxy()->calculateSizes(commandPool, commandPoolSize, dataSize, this);
    }
}
