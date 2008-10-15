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

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/convergence/ChannelState.hpp>
#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>
#include <WIFIMAC/helper/Keys.hpp>
#include <WIFIMAC/Layer2.hpp>

#include <DLL/Layer2.hpp>

#include <WNS/ldk/FlowGate.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::Manager,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.Manager",
    wns::ldk::FUNConfigCreator);

Manager::Manager(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config) :
    wns::ldk::fu::Plain<Manager, ManagerCommand>(fun),
    config_(config),
    logger_(config_.get("logger")),
    expectedACKDuration(config.get<wns::simulator::Time>("myConfig.expectedACKDuration")),
    sifsDuration(config.get<wns::simulator::Time>("myConfig.sifsDuration")),
    myMACAddress_(config.get<wns::service::dll::UnicastAddress>("myMACAddress")),
    ucName_(config_.get<std::string>("upperConvergenceCommandName")),
    numAntennas(config_.get<int>("myConfig.numAntennas"))
{
    MESSAGE_SINGLE(NORMAL, logger_, "created");
    friends.phyUser = NULL;
    friends.upperConvergence = NULL;
}

Manager::~Manager()
{

}

void
Manager::onFUNCreated()
{
    // set services in my PhyUser to communicate with lower layer
    friends.phyUser = getFUN()->findFriend<wifimac::convergence::PhyUser*>(config_.get<std::string>("phyUserName"));

    friends.phyUser->setDataTransmissionService(
        this->getFUN()->getLayer<dll::Layer2*>()
        ->getService<wns::service::phy::ofdma::DataTransmission*>( config_.get<std::string>("phyDataTransmission") ) );
    friends.phyUser->setNotificationService(
        this->getFUN()->getLayer<dll::Layer2*>()
        ->getService<wns::service::phy::ofdma::Notification*>( config_.get<std::string>("phyNotification") ) );

    // set service in ChannelState to observe the CarrierSense
    wifimac::convergence::ChannelState* cs = getFUN()->findFriend<wifimac::convergence::ChannelState*>(config_.get<std::string>("channelStateName"));
    assure(cs, "Could not get ChannelState from my FUN");

    cs->setCarrierSensingService(
        this->getFUN()->getLayer<dll::Layer2*>()
        ->getService<wns::service::phy::ofdma::Notification*>(config_.get<std::string>("phyCarrierSense")) );

    // open the flow gate for my compounds
    wns::ldk::FlowGate* rxFilter = getFUN()->findFriend<wns::ldk::FlowGate*>(config_.get<std::string>("receiveFilterName"));
    assure(rxFilter, "Could not get receiveFilter from my FUN");
    rxFilter->createFlow(wns::ldk::ConstKeyPtr(new wifimac::helper::LinkByTransmitter(myMACAddress_)));
    wns::ldk::ConstKeyPtr key(new wifimac::helper::LinkByTransmitter(myMACAddress_));
    rxFilter->openFlow(key);

    // Register manager at Layer2
    this->getFUN()->getLayer<wifimac::Layer2*>()->registerManager(this, myMACAddress_);

    // find upper convergence
    friends.upperConvergence = getFUN()->findFriend<dll::UpperConvergence*>(ucName_);

    // set the number of antennas for MIMO
    assure(wifimac::management::TheVCIBService::Instance().getVCIB(), "No virtual capability information base service found");
    wifimac::management::TheVCIBService::Instance().getVCIB()->set<int>(myMACAddress_, "numAntennas", numAntennas);
}


void
Manager::processIncoming(const wns::ldk::CompoundPtr& /*compound*/)
{

}

void
Manager::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    assure(getFUN()->getCommandReader(ucName_)
           ->readCommand<dll::UpperCommand>(compound->getCommandPool())
           ->peer.sourceMACAddress == myMACAddress_,
           "Try to tx compound with source address " <<
           getFUN()->getCommandReader(ucName_)
           ->readCommand<dll::UpperCommand>(compound->getCommandPool())
           ->peer.sourceMACAddress <<
           " from transceiver with MAC address " << myMACAddress_ );

    // The command has to be activated to be considered in the createReply chain
    ManagerCommand* mc = activateCommand(compound->getCommandPool());
    mc->peer.type = DATA;
    mc->peer.frameExchangeDuration = this->sifsDuration + this->expectedACKDuration;
}

wifimac::convergence::PhyUser*
Manager::getPhyUser()
{
    if(friends.phyUser)
    {
        return(friends.phyUser);
    }
    else
    {
        return(getFUN()->findFriend<wifimac::convergence::PhyUser*>(config_.get<std::string>("phyUserName")));
    }
}

dll::Layer2::StationType
Manager::getStationType() const
{
    // simply relayed to Layer2
    return(this->getFUN()->getLayer<dll::Layer2*>()->getStationType());
}

wns::service::dll::UnicastAddress
Manager::getMACAddress() const
{
	return(this->myMACAddress_);
}

void
Manager::associateWith(wns::service::dll::UnicastAddress svrAddress)
{
	assure(this->getFUN()->getLayer<dll::Layer2*>()->getStationType() == dll::StationTypes::UT(),
		   "Called associateWith by non-UT");

	// the association links the layer2 of this STA and the layer2 of the AP -
	// not the transceiver
	this->getFUN()->getLayer<dll::Layer2*>()->getControlService<dll::services::control::Association>("ASSOCIATION")
		->associate(this->myMACAddress_, svrAddress);

}

wns::ldk::CommandPool*
Manager::createReply(const wns::ldk::CommandPool* original) const
{
	wns::ldk::CommandPool* reply = this->getFUN()->getProxy()->createReply(original, this);

	dll::UpperCommand* ucReply = getFUN()->getProxy()->getCommand<dll::UpperCommand>(reply, ucName_);
	dll::UpperCommand* ucOriginal = getFUN()->getCommandReader(ucName_)->readCommand<dll::UpperCommand>(original);

	// this cannot be set to this->myAddress_, because it is not defined which
	// Manager-entity will be asked for createReply
	ucReply->peer.sourceMACAddress = ucOriginal->peer.targetMACAddress;

    ManagerCommand* mc = activateCommand(reply);
    mc->peer.type = DATA;

	MESSAGE_SINGLE(NORMAL, logger_, "create reply done, set sourceMACAddress to " << ucReply->peer.sourceMACAddress);

	return(reply);
}

wns::service::dll::UnicastAddress
Manager::getTransmitterAddress(const wns::ldk::CommandPool* commandPool) const
{
    return (getFUN()->getCommandReader(ucName_)->readCommand<dll::UpperCommand>(commandPool)->peer.sourceMACAddress);
}

wns::service::dll::UnicastAddress
Manager::getReceiverAddress(const wns::ldk::CommandPool* commandPool) const
{
    return (getFUN()->getCommandReader(ucName_)->readCommand<dll::UpperCommand>(commandPool)->peer.targetMACAddress);
}

bool
Manager::isForMe(const wns::ldk::CommandPool* commandPool) const
{
    if(getFUN()->getCommandReader(ucName_)->commandIsActivated(commandPool))
    {
        return(getFUN()->getCommandReader(ucName_)->readCommand<dll::UpperCommand>(commandPool)->peer.targetMACAddress == this->myMACAddress_);
    }
    else
    {
        // broadcast -> is for me
        return true;
    }
}

wifimac::FrameType
Manager::getFrameType(const wns::ldk::CommandPool* commandPool) const
{
    return(getCommand(commandPool)->peer.type);
}

wns::ldk::CompoundPtr
Manager::createCompound(const wns::service::dll::UnicastAddress transmitterAddress,
                                const wns::service::dll::UnicastAddress receiverAddress,
                                const FrameType type,
                                const wns::simulator::Time frameExchangeDuration,
                                const bool requiresDirectReply)
{
    wns::ldk::CompoundPtr compound(new wns::ldk::Compound(getFUN()->getProxy()->createCommandPool()));
    ManagerCommand* mc = activateCommand(compound->getCommandPool());
    dll::UpperCommand* uc = friends.upperConvergence->activateCommand(compound->getCommandPool());

    uc->peer.sourceMACAddress = transmitterAddress;
    uc->peer.targetMACAddress = receiverAddress;
    mc->peer.type = type;
    mc->peer.frameExchangeDuration = frameExchangeDuration;
    mc->peer.requiresDirectReply = requiresDirectReply;

    return(compound);
}

void
Manager::setFrameType(const wns::ldk::CommandPool* commandPool, const FrameType type)
{
    getCommand(commandPool)->peer.type = type;
}

wifimac::convergence::PhyMode
Manager::getPhyMode(const wns::ldk::CommandPool* commandPool) const
{
    return(getCommand(commandPool)->getPhyMode());
}

void
Manager::setPhyMode(const wns::ldk::CommandPool* commandPool, const wifimac::convergence::PhyMode phyMode)
{
    getCommand(commandPool)->peer.phyMode = phyMode;
}

void
Manager::setPhyMode(const wns::ldk::CommandPool* commandPool, const int phyModeId)
{
    this->setPhyMode(commandPool, this->getPhyUser()->getPhyModeProvider()->getPhyMode(phyModeId));
}

wns::simulator::Time
Manager::getFrameExchangeDuration(const wns::ldk::CommandPool* commandPool) const
{
    return(getCommand(commandPool)->peer.frameExchangeDuration);
}

void
Manager::setFrameExchangeDuration(const wns::ldk::CommandPool* commandPool, const wns::simulator::Time duration)
{
    getCommand(commandPool)->peer.frameExchangeDuration = duration;
}

bool
Manager::getRequiresDirectReply(const wns::ldk::CommandPool* commandPool) const
{
    return(getCommand(commandPool)->peer.requiresDirectReply);
}

void
Manager::setRequiresDirectReply(const wns::ldk::CommandPool* commandPool, bool requiresDirectReply)
{
    getCommand(commandPool)->peer.requiresDirectReply = requiresDirectReply;
}

unsigned int
Manager::getNumAntennas() const
{
    return this->numAntennas;
}
