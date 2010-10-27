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

#include <WIFIMAC/Layer2.hpp>
#include <WIFIMAC/helper/DestinationSortedWindowProbe.hpp>
#include <WIFIMAC/pathselection/IPathSelection.hpp>

#include <WNS/StaticFactory.hpp>
#include <WNS/probe/bus/utils.hpp>
#include <WNS/service/dll/StationTypes.hpp>

using namespace wifimac::helper;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    DestinationSortedWindowProbe,
    wns::ldk::probe::Probe,
    "wifimac.helper.DestinationSortedWindowProbe",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    DestinationSortedWindowProbe,
    wns::ldk::FunctionalUnit,
    "wifimac.helper.DestinationSortedWindowProbe",
    wns::ldk::FUNConfigCreator);

DestinationSortedWindowProbe::DestinationSortedWindowProbe(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::probe::bus::Window(fun, config_),
    config(config_),
    ucReader(NULL),
    windowSize(config.get<simTimeType>("windowSize")),
    ucCommandName(config.get<std::string>("ucCommandName"))
{
    // read the localContext from the config
    wns::probe::bus::ContextProviderCollection localContext(&fun->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config.len("localIDs.keys()"); ++ii)
    {
        std::string key = config.get<std::string>("localIDs.keys()",ii);
        unsigned long int value  = config.get<unsigned long int>("localIDs.values()",ii);
        localContext.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
    }

    this->bitsIncoming = wns::probe::bus::collector(localContext, config, "incomingBitDestinationSortedWindowProbeName");
    this->bitsAggregated = wns::probe::bus::collector(localContext, config, "aggregatedBitDestinationSortedWindowProbeName");
    this->bitsOutgoing = wns::probe::bus::collector(localContext, config, "outgoingBitDestinationSortedWindowProbeName");
    this->relativeGoodput = wns::probe::bus::collector(localContext, config, "relativeGoodputDestinationSortedWindowProbeName");
}

DestinationSortedWindowProbe::~DestinationSortedWindowProbe()
{
}

void
DestinationSortedWindowProbe::onFUNCreated()
{
    // Obtain upper convergence command reader
    ucReader = getFUN()->getCommandReader(ucCommandName);
}

void
DestinationSortedWindowProbe::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    // read the addresses
    unsigned int sourceAddress =
        ucReader->readCommand<dll::UpperCommand>(compound->getCommandPool())->peer.sourceMACAddress.getInteger();
    unsigned int destAddress =
        getFUN()->getLayer<wifimac::Layer2*>()->getDLLAddress().getInteger();

    Bit commandPoolSize;
    Bit dataSize;
    this->getFUN()->calculateSizes(compound->getCommandPool(), commandPoolSize, dataSize, this);
    const long int compoundLength = commandPoolSize + dataSize;

    this->putProbe(bitsIncomingProbeHolder, sourceAddress, compoundLength);

    wns::ldk::probe::bus::WindowCommand* command = this->getCommand(compound->getCommandPool());
    wifimac::helper::DestinationSortedWindowProbe* peerFU = dynamic_cast<wifimac::helper::DestinationSortedWindowProbe*>(command->magic.probingFU);
    assure(peerFU != NULL, "Expected wifimac::helper::DestinationSortedWindowProbe as peer!");

    peerFU->putAggregatedProbe(destAddress, compoundLength);

    // Now do the general (base class stuff)
    this->wns::ldk::probe::bus::Window::processIncoming(compound);
}

void
DestinationSortedWindowProbe::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    // First do the general (base class) stuff
    this->wns::ldk::probe::bus::Window::processOutgoing(compound);

    wns::service::dll::UnicastAddress destAdr =
        ucReader->readCommand<dll::UpperCommand>(compound->getCommandPool())->peer.targetMACAddress;

    if((not destAdr.isValid()) and
       (this->getFUN()->getLayer<dll::ILayer2*>()->getStationType() == wns::service::dll::StationTypes::UT()))
    {
        // no destination address and STA -> destination is AP
        assure(this->getFUN()->getLayer<dll::ILayer2*>()->getControlService<dll::services::control::Association>("ASSOCIATION")->hasAssociation(),
               "Outgoing data, but no association");
        destAdr =
        this->getFUN()->getLayer<dll::ILayer2*>()->getControlService<dll::services::control::Association>("ASSOCIATION")->getAssociation();
    }

    if((not destAdr.isValid()) and
       (this->getFUN()->getLayer<dll::ILayer2*>()->getStationType() == wns::service::dll::StationTypes::FRS()))
    {
        // no destination address and MP -> destination is portal
        wns::service::dll::UnicastAddress sourceAdr =
            ucReader->readCommand<dll::UpperCommand>(compound->getCommandPool())->peer.sourceMACAddress;
        destAdr = this->getFUN()->getLayer<wifimac::Layer2*>()->getManagementService<wifimac::pathselection::IPathSelection>("PATHSELECTIONOVERVPS")->getPortalFor(sourceAdr);
        assure(destAdr.isValid(), "Could not get destAdr from ps service");
    }

    assure(destAdr.isValid(),
           "Unable to get destination address!");

    Bit commandPoolSize;
    Bit dataSize;
    this->getFUN()->calculateSizes(compound->getCommandPool(), commandPoolSize, dataSize, this);
    const long int compoundLength = commandPoolSize + dataSize;

    this->putProbe(bitsOutgoingProbeHolder, destAdr.getInteger(), compoundLength);
}

void
DestinationSortedWindowProbe::putAggregatedProbe(unsigned int adr,
                                                 double value)
{
    if(this->getFUN()->getLayer<dll::ILayer2*>()->getStationType() == wns::service::dll::StationTypes::UT())
    {
        // The STA can only have the associated AP/MP as peer --> change the adr
        adr = this->getFUN()->getLayer<dll::ILayer2*>()->getControlService<dll::services::control::Association>("ASSOCIATION")->getAssociation().getInteger();
    }
    this->putProbe(bitsAggregatedProbeHolder, adr, value);
}

void
DestinationSortedWindowProbe::putProbe(SlidingWindowMap& probeHolder,
                                       unsigned int adr,
                                       double value)
{
    if(not probeHolder.knows(adr))
    {
        probeHolder.insert(adr, new wns::SlidingWindow(windowSize));
    }
    probeHolder.find(adr)->put(value);
}

void
DestinationSortedWindowProbe::periodically()
{

    storeProbes(bitsIncomingProbeHolder, bitsIncoming);
    storeProbes(bitsAggregatedProbeHolder, bitsAggregated);
    storeProbes(bitsOutgoingProbeHolder, bitsOutgoing);

    // relative goodput: aggregated/outgoing to one destination

    for(SlidingWindowMap::const_iterator outItr = bitsOutgoingProbeHolder.begin();
        outItr != bitsOutgoingProbeHolder.end();
        ++outItr)
    {
        bool found = false;
        for(SlidingWindowMap::const_iterator aggItr = bitsAggregatedProbeHolder.begin();
            aggItr != bitsAggregatedProbeHolder.end();
            ++aggItr)
        {
            if(aggItr->first == outItr->first)
            {
                // set the correct context
                if(outItr->second->getPerSecond() > 0)
                {
                    this->getFUN()->getLayer<wifimac::Layer2*>()->updateContext("MAC.WindowProbeAddress", aggItr->first);
                    relativeGoodput->put(aggItr->second->getPerSecond() / outItr->second->getPerSecond());
                }
                found = true;
                break;
            }
        }
        if(not found and (outItr->second->getPerSecond() > 0))
        {
            // outgoing, but no aggregated
            this->getFUN()->getLayer<wifimac::Layer2*>()->updateContext("MAC.WindowProbeAddress", outItr->first);
            relativeGoodput->put(0.0);
        }
    }

    // Call base class function
    this->wns::ldk::probe::bus::Window::periodically();
}

void
DestinationSortedWindowProbe::storeProbes(SlidingWindowMap& probeHolder, wns::probe::bus::ContextCollectorPtr& putter)
{
    for(SlidingWindowMap::const_iterator probeItr = probeHolder.begin(); probeItr != probeHolder.end(); ++probeItr)
    {
        // set the correct context
        this->getFUN()->getLayer<wifimac::Layer2*>()->updateContext("MAC.WindowProbeAddress", probeItr->first);

        putter->put(probeItr->second->getPerSecond());
    }
}
