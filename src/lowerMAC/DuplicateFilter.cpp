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

#include <WIFIMAC/lowerMAC/DuplicateFilter.hpp>

#include <WNS/ldk/arq/StopAndWait.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	DuplicateFilter,
	wns::ldk::FunctionalUnit,
	"wifimac.lowerMAC.DuplicateFilter",
	wns::ldk::FUNConfigCreator);

DuplicateFilter::DuplicateFilter(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config):
	wns::ldk::fu::Plain<DuplicateFilter, DuplicateFilterCommand>(fun),

	logger(config.get("logger")),
    nextSN(1),
    managerName(config.get<std::string>("managerName")),
    arqCommandName(config.get<std::string>("arqCommandName"))
{

}

DuplicateFilter::~DuplicateFilter()
{

}

void DuplicateFilter::onFUNCreated()
{
	friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
	assure(friends.manager, "Management entity not found");
}

void DuplicateFilter::doSendData(const wns::ldk::CompoundPtr& compound)
{
	// add duplicate filter command
    DuplicateFilterCommand* command = activateCommand(compound->getCommandPool());
    command->peer.sn = nextSN;
    ++nextSN;

	getConnector()->getAcceptor(compound)->sendData(compound);
}

void DuplicateFilter::doOnData(const wns::ldk::CompoundPtr& compound)
{
    // retransmission, check sequence number
    DuplicateFilterCommand* command = getCommand(compound);

    if(not lastReceivedSN.knows(friends.manager->getTransmitterAddress(compound->getCommandPool())))
    {
        // first compound from source
        MESSAGE_SINGLE(NORMAL, logger, "Received first frame from " << friends.manager->getTransmitterAddress(compound->getCommandPool()) << " -> deliver");
        lastReceivedSN.insert(friends.manager->getTransmitterAddress(compound->getCommandPool()), command->peer.sn);
        getDeliverer()->getAcceptor(compound)->onData(compound);
    }
    else
    {
        if(lastReceivedSN.find(friends.manager->getTransmitterAddress(compound->getCommandPool())) != command->peer.sn)
        {
            // compound has different sn
            MESSAGE_SINGLE(NORMAL, logger, "Received frame from " << friends.manager->getTransmitterAddress(compound->getCommandPool()) << " with unknown sn -> deliver");
            getDeliverer()->getAcceptor(compound)->onData(compound);
            lastReceivedSN.update(friends.manager->getTransmitterAddress(compound->getCommandPool()), command->peer.sn);
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, logger, "Received duplicate frame from " << friends.manager->getTransmitterAddress(compound->getCommandPool()) << " -> drop");
        }
    }
}

bool DuplicateFilter::doIsAccepting(const wns::ldk::CompoundPtr& compound) const
{
	return getConnector()->hasAcceptor(compound);
}

void DuplicateFilter::doWakeup()
{
	// simply forward the wakeup call
	getReceptor()->wakeup();
}
