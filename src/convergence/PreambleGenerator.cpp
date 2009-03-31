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

#include <WIFIMAC/convergence/PreambleGenerator.hpp>
#include <DLL/Layer2.hpp>

using namespace wifimac::convergence;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	wifimac::convergence::PreambleGenerator,
	wns::ldk::FunctionalUnit,
	"wifimac.convergence.PreambleGenerator",
	wns::ldk::FUNConfigCreator);

PreambleGenerator::PreambleGenerator(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
	wns::ldk::fu::Plain<PreambleGenerator, PreambleGeneratorCommand>(fun),
    phyUserName(config_.get<std::string>("phyUserName")),
    protocolCalculatorName(config_.get<std::string>("protocolCalculatorName")),
    managerName(config_.get<std::string>("managerName")),
	logger(config_.get("logger")),
    pendingCompound(),
    pendingPreamble()
{
	MESSAGE_SINGLE(NORMAL, this->logger, "created");
    friends.manager = NULL;
    friends.phyUser = NULL;
    protocolCalculator = NULL;
}

PreambleGenerator::~PreambleGenerator()
{

}

void
PreambleGenerator::onFUNCreated()
{
	MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.phyUser = getFUN()->findFriend<wifimac::convergence::PhyUser*>(phyUserName);
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
    protocolCalculator = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
}

void
PreambleGenerator::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    if(friends.manager->getFrameType(compound->getCommandPool()) == PREAMBLE)
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "Received PREAMBLE -> drop");
    }
    else
    {
        // deliver frame
        getDeliverer()->getAcceptor(compound)->onData(compound);
    }
}

void
PreambleGenerator::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    // compute transmission duration of the frame, dependent on the mcs
    wifimac::convergence::PhyMode phyMode = friends.manager->getPhyMode(compound->getCommandPool());
    wns::simulator::Time frameTxDuration = protocolCalculator->getDuration()->getMPDU_PPDU(compound->getLengthInBits(),
                                                                                           phyMode.getDataBitsPerSymbol(),
                                                                                           phyMode.getNumberOfSpatialStreams(),
                                                                                           20, std::string("Basic"))
        - protocolCalculator->getDuration()->getPreamble(phyMode.getNumberOfSpatialStreams(),std::string("Basic"));

    // First we generate a preamble
    this->pendingPreamble = compound->copy();
    friends.manager->setFrameType(this->pendingPreamble->getCommandPool(),
                                  PREAMBLE);
    friends.manager->setPhyMode(this->pendingPreamble->getCommandPool(),
                                friends.phyUser->getPhyModeProvider()->getPreamblePhyMode());
    friends.manager->setFrameExchangeDuration(this->pendingPreamble->getCommandPool(),
                                              frameTxDuration);
    PreambleGeneratorCommand* preambleCommand = activateCommand(this->pendingPreamble->getCommandPool());
    preambleCommand->peer.frameId = compound->getBirthmark();

    MESSAGE_SINGLE(NORMAL, logger, "Outgoing preamble with frame tx duration " << frameTxDuration);

    this->pendingCompound = compound;
}

const wns::ldk::CompoundPtr
PreambleGenerator::hasSomethingToSend() const
{
    if(this->pendingPreamble)
    {
        assure(this->pendingCompound, "Pending preamble but no pending compound");
        return(this->pendingPreamble);
    }
    return(this->pendingCompound);
}

wns::ldk::CompoundPtr
PreambleGenerator::getSomethingToSend()
{
    assure(this->pendingCompound, "Called getSomethingToSend without pending compound");

    wns::ldk::CompoundPtr myFrame;
    if(this->pendingPreamble)
    {
        assure(this->pendingCompound, "Pending preamble but no pending compound");
        myFrame = this->pendingPreamble;
        this->pendingPreamble = wns::ldk::CompoundPtr();
        MESSAGE_SINGLE(NORMAL, logger, "Sending preamble");
    }
    else
    {
        myFrame = this->pendingCompound;
        this->pendingCompound = wns::ldk::CompoundPtr();
        MESSAGE_SINGLE(NORMAL, logger, "Sending psdu");
    }
    return(myFrame);
}

bool
PreambleGenerator::hasCapacity() const
{
    return(this->pendingCompound == wns::ldk::CompoundPtr());
}

