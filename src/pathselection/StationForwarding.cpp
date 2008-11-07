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

#include <WIFIMAC/pathselection/StationForwarding.hpp>
#include <WIFIMAC/Layer2.hpp>

#include <WNS/ldk/CommandPool.hpp>
#include <WNS/Exception.hpp>

using namespace wifimac::pathselection;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	wifimac::pathselection::StationForwarding,
	wns::ldk::FunctionalUnit,
	"wifimac.pathselection.StationForwarding",
	wns::ldk::FUNConfigCreator);

StationForwarding::StationForwarding(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
	wns::ldk::fu::Plain<StationForwarding, ForwardingCommand>(fun),
	config(config_),
	logger(config.get("logger")),
	ucName(config.get<std::string>("upperConvergenceName"))
{
	MESSAGE_SINGLE(NORMAL, this->logger, "created");
}

StationForwarding::~StationForwarding()
{

}

void
StationForwarding::onFUNCreated()
{
    layer2 = getFUN()->getLayer<wifimac::Layer2*>();

	if(layer2->getStationType() != dll::StationTypes::UT())
	{
		throw wns::Exception("StationForwarding is only allowed for StationType = UT");
	}
}

bool
StationForwarding::doIsAccepting(const wns::ldk::CompoundPtr& _compound) const
{

	if(layer2->getControlService<dll::services::control::Association>("ASSOCIATION")->hasAssociation())
	{
		return getConnector()->hasAcceptor(_compound);
	}
	else
	{
		// No association, no delivery, no acceptance
		return false;
	}
}

void
StationForwarding::doWakeup()
{
	// simply forward the wakeup call
	getReceptor()->wakeup();
}

void
StationForwarding::doOnData(const wns::ldk::CompoundPtr& _compound)
{
	// Received compound from a transceiver

	// First: copy the compound
	wns::ldk::CompoundPtr compound = _compound->copy();

	ForwardingCommand* fc = getCommand(compound);
	dll::UpperCommand* uc = getFUN()->getProxy()->getCommand<dll::UpperCommand>(compound->getCommandPool(), ucName);

	++fc->magic.hopCount;
	fc->magic.path.push_back(layer2->getDLLAddress());


	MESSAGE_BEGIN(NORMAL, this->logger, m, "doOnData with fromDS " << fc->peer.fromDS << " toDS " << fc->peer.toDS);
	if(fc->peer.fromDS == true)
	{
		m << " " << fc->peer.originalSource << " ->";
	}
	if(fc->peer.addressExtension == true)
	{
		m << " " << fc->peer.meshSource << " ->";
	}
	m << " " << uc->peer.sourceMACAddress << " -> " << uc->peer.targetMACAddress << " -> ";
	if(fc->peer.addressExtension == true)
	{
		m << " " << fc->peer.meshDestination << " ->";
	}
	if(fc->peer.toDS == true)
	{
		m << fc->peer.finalDestination;
	}
	m << " hc: " << fc->magic.hopCount;
	MESSAGE_END();

	// fromDS, the first source is saved in the fc
	assure(fc->peer.fromDS, "received frame NOT from DS");
	uc->peer.sourceMACAddress = fc->peer.originalSource;

	getDeliverer()->getAcceptor(compound)->onData(compound);

}

void
StationForwarding::doSendData(const wns::ldk::CompoundPtr& _compound)
{
	// First: copy the compound
	wns::ldk::CompoundPtr compound = _compound->copy();

	// Received fresh packet from upper layer -> activate command
	ForwardingCommand* fc = activateCommand(compound->getCommandPool());
	// Get the upperCommand, which contains the final source and destination
	dll::UpperCommand* uc = getFUN()->getProxy()->getCommand<dll::UpperCommand>(compound->getCommandPool(), ucName);

	fc->magic.isUplink = true;
	fc->peer.toDS = true;
	fc->peer.fromDS = false;
	fc->peer.finalDestination = uc->peer.targetMACAddress;
	uc->peer.sourceMACAddress = layer2->getDLLAddress();
	uc->peer.targetMACAddress = layer2->getControlService<dll::services::control::Association>("ASSOCIATION")->getAssociation();

	fc->magic.path.push_back(layer2->getDLLAddress());

	getConnector()->getAcceptor(compound)->sendData(compound);
}
