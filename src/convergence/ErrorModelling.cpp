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

#include <WIFIMAC/convergence/ErrorModelling.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>

using namespace wifimac::convergence;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	wifimac::convergence::ErrorModelling,
	wns::ldk::FunctionalUnit,
	"wifimac.convergence.ErrorModelling",
	wns::ldk::FUNConfigCreator);

ErrorModelling::ErrorModelling(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& _config) :
	wns::ldk::fu::Plain<ErrorModelling, ErrorModellingCommand>(fun),
	config(_config),
	logger(config.get<wns::pyconfig::View>("logger")),
	phyUserCommandName(config.get<std::string>("phyUserCommandName")),
	managerCommandName(config.get<std::string>("managerCommandName"))
{

}

void ErrorModelling::processIncoming(const wns::ldk::CompoundPtr& compound)
{
	wns::Ratio sinr = getFUN()->getCommandReader(phyUserCommandName)->
		readCommand<wifimac::convergence::CIRProviderCommand>(compound->getCommandPool())->getCIR();
	wns::Power rss = getFUN()->getCommandReader(phyUserCommandName)->
		readCommand<wifimac::convergence::CIRProviderCommand>(compound->getCommandPool())->getRSS();
	wifimac::convergence::PhyMode phyMode = getFUN()->getCommandReader(managerCommandName)->
        readCommand<wifimac::lowerMAC::ManagerCommand>(compound->getCommandPool())->getPhyMode();

	ErrorModellingCommand* emc = activateCommand(compound->getCommandPool());

	Bit commandPoolSize = 0;
	Bit dataSize = 0;
	this->calculateSizes(compound->getCommandPool(), commandPoolSize, dataSize);

	emc->local.per = phyMode.getPer(rss, sinr, commandPoolSize + dataSize);

	MESSAGE_SINGLE(NORMAL, logger, "New compound with SNR " << sinr << " phyMode " << phyMode << " --> per " << emc->local.per);
}

void ErrorModelling::processOutgoing(const wns::ldk::CompoundPtr& /*compound*/)
{

}
