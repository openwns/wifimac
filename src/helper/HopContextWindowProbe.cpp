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

#include <WIFIMAC/helper/HopContextWindowProbe.hpp>
#include <WIFIMAC/pathselection/ForwardingCommand.hpp>
#include <WIFIMAC/Layer2.hpp>

#include <WNS/StaticFactory.hpp>
#include <WNS/probe/bus/utils.hpp>

using namespace wifimac::helper;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	HopContextWindowProbe,
	wns::ldk::probe::Probe,
	"wifimac.helper.HopContextWindowProbe",
	wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	HopContextWindowProbe,
	wns::ldk::FunctionalUnit,
	"wifimac.helper.HopContextWindowProbe",
	wns::ldk::FUNConfigCreator);

HopContextWindowProbe::HopContextWindowProbe(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config) :
	wns::ldk::probe::bus::Window(fun, config),
	config_(config),
	forwardingReader(NULL),
	windowSize(config_.get<simTimeType>("windowSize"))
{
	// read the localContext from the config
	wns::probe::bus::ContextProviderCollection localContext(&fun->getLayer()->getContextProviderCollection());
	for (int ii = 0; ii<config.len("localIDs.keys()"); ++ii)
	{
		std::string key = config.get<std::string>("localIDs.keys()",ii);
		unsigned long int value  = config.get<unsigned long int>("localIDs.values()",ii);
		localContext.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
	}

	this->hopCountedBitsIncoming = wns::probe::bus::collector(localContext, config, "incomingHopCountedBitThroughputProbeName");
	this->hopCountedCompoundsIncoming = wns::probe::bus::collector(localContext, config, "incomingHopCountedCompoundThroughputProbeName");
	this->hopCountedBitsAggregated = wns::probe::bus::collector(localContext, config, "aggregatedHopCountedBitThroughputProbeName");
	this->hopCountedCompoundsAggregated = wns::probe::bus::collector(localContext, config, "aggregatedHopCountedCompoundThroughputProbeName");
}

HopContextWindowProbe::~HopContextWindowProbe()
{
	forwardingReader = NULL;
}

void
HopContextWindowProbe::onFUNCreated()
{
	// Obtain MACg Command Reader
	forwardingReader = getFUN()->getCommandReader(config_.get<std::string>("forwardingCommandName"));
}

void
HopContextWindowProbe::processIncoming(const wns::ldk::CompoundPtr& compound)
{
	// How many Hops has this Compound traveled?
	unsigned int numHops =
		(forwardingReader->readCommand<wifimac::pathselection::ForwardingCommand>(compound->getCommandPool()))
		->magic.hopCount;

	Bit commandPoolSize;
	Bit dataSize;
	this->getFUN()->calculateSizes(compound->getCommandPool(), commandPoolSize, dataSize, this);
	const long int compoundLength = commandPoolSize + dataSize;

	this->putProbe(this->bitsIncomingProbeHolder, numHops, compoundLength);
	this->putProbe(this->compoundsIncomingProbeHolder, numHops, 1);

	wns::ldk::probe::bus::WindowCommand* command = this->getCommand(compound->getCommandPool());
	wifimac::helper::HopContextWindowProbe* peerFU = dynamic_cast<wifimac::helper::HopContextWindowProbe*>(command->magic.probingFU);
	assure(peerFU != NULL, "Expected wifimac::helper::HopContextWindowProbe as peer!");

	// put aggregated probes at peer
	peerFU->putProbe(peerFU->bitsAggregatedProbeHolder, numHops, compoundLength);
	peerFU->putProbe(peerFU->compoundsAggregatedProbeHolder, numHops, 1);

        // Now do the general (base class stuff)
	this->wns::ldk::probe::bus::Window::processIncoming(compound);
}

void
HopContextWindowProbe::putProbe(SlidingWindowMap& probeHolder, unsigned int numHops, double value)
{
	if (! probeHolder.knows(numHops))
	{
		// A yet unseen numHops occured --> create Sliding Window
		probeHolder.insert(numHops, new wns::SlidingWindow(windowSize));
	}
	probeHolder.find(numHops)->put(value);
}

void
HopContextWindowProbe::periodically()
{
	assure(bitsIncomingProbeHolder.size() == compoundsIncomingProbeHolder.size(),
	       "Different number of different hop-counts for bitsIncoming and compoundsIncoming");
	storeProbes(bitsIncomingProbeHolder, hopCountedBitsIncoming);
	storeProbes(compoundsIncomingProbeHolder, hopCountedCompoundsIncoming);

	assure(bitsAggregatedProbeHolder.size() == compoundsAggregatedProbeHolder.size(),
	       "Different number of different hop-counts for bitsAggregated and compoundsAggregated");
	storeProbes(bitsAggregatedProbeHolder, hopCountedBitsAggregated);
	storeProbes(compoundsAggregatedProbeHolder, hopCountedCompoundsAggregated);

	// Call base class function
	this->wns::ldk::probe::bus::Window::periodically();
}

void
HopContextWindowProbe::storeProbes(SlidingWindowMap& probeHolder, wns::probe::bus::ContextCollectorPtr& putter)
{
	for(SlidingWindowMap::const_iterator probeItr = probeHolder.begin(); probeItr != probeHolder.end(); ++probeItr)
	{
		unsigned int numHops = probeItr->first;

		// set the correct hop-count context
		this->getFUN()->getLayer<wifimac::Layer2*>()->updateContext("MAC.WindowProbeHopCount", numHops);

		// and put the result
		putter->put(probeItr->second->getPerSecond());
	}
}
