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

#include <WNS/ldk/Layer.hpp>
#include <WNS/StaticFactory.hpp>
#include <WNS/probe/IDProviderRegistry.hpp>
#include <WNS/probe/utils.hpp>

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

HopContextWindowProbe::HopContextWindowProbe(wns::ldk::fun::FUN* fuNet, const wns::pyconfig::View& config) :
	wns::ldk::probe::bus::Window(fuNet, config),
	config_(config),
	forwardingReader(NULL),
	windowSize(config_.get<simTimeType>("windowSize")),
{
	// read the localIDs from the config
	localIDs(&fuNet->getLayer()->getContextProviderCollection());
	for (int ii = 0; ii<config_.len("localIDs.keys()"); ++ii)
	{
		std::string key = config_.get<std::string>("localIDs.keys()",ii);
		uint32_t value  = config_.get<uint32_t>("localIDs.values()",ii);
		localIDs.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
	}
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

	if(!config_.isNone("managerName")
	{
		// If the probe is above the transceiver-split, there will be no manager[name]
		friends.manager = getFUN()->findFriend<wifimac::multiradio::Manager*>(config_.get<std::string>("managerName"));
		assure(friends.manager, "Management entity not found");
	}
}

void
HopContextWindowProbe::processIncoming(const wns::ldk::CompoundPtr& compount)
{
	// How many Hops has this Compound traveled?
	unsigned int numHops =
		(forwardingReader->readCommand<wifimac::pathselection::ForwardingCommand>(compound->getCommandPool()))
		->magic.hopCount;

	Bit commandPoolSize;
	Bit dataSize;
	this->getFUN()->calculateSizes(compound->getCommandPool(), commandPoolSize, dataSize, this);
	const int32_t compoundLength = commandPoolSize + dataSize;

	this->putIntoBitsIncomming(numHops, compoundLength);
	this->putIntoCompoundsIncomming(numHops, 1);

	wns::ldk::probe::WindowCommand* command = this->getCommand(compound->getCommandPool());
	wifimac::helper::HopContextWindowProbe* peerFU = dynamic_cast<wifimac::helper::HopContextWindowProbe*>(command->magic.probingFU);
	assure(peerFU != NULL, "Expected wifimac::helper::HopContextWindowProbe as peer!");

	peerFU->putIntoBitsAggregated(numHops, compoundLength);
	peerFU->putIntoCompoundsAggregated(numHops, 1);

        // Now do the general (base class stuff)
	this->wns::ldk::probe::bus::Window::processIncoming(compound);
}

wns::SlidingWindow*
HopContextWindowProbe::getProbe(SlidingWindowMap& probeHolder, PutterMap& putterHolder, std::string id, unsigned int numHops)
{
	if (! probeHolder.knows(numHops))
	{
		assure(!probePutter.knows(numHops), "probePutter already knows " << numHops << " hops, but probeHolder does not");

		// A yet unseen numHops occured --> create Sliding Window and
		// associated Putter
		probeHolder.insert(numHops, new wns::SlidingWindow(windowSize));
		wns::probe::bus::ContextProviderCollection hopWiseLocalIDs(localIDs);
		hopWiseLocalIDs.addProvider(wns::probe::bus::contextprovider::Constant("MAC.HopCount", numHops));
		probePutter.insert(numHops, new wns::probe::bus::collector(hopWiseLocalIDs, config_, id));
	}
	assure(probeHolder.knows(numHops), "probeContainer for " << numHops << " hops not found!");
	return(probeHolder.find(numHops));
}

void
HopContextWindowProbe::putIntoBitsIncomming(unsigned int numHops, double value)
{
	getProbe(this->bitsIncommingProbeHolder,
		 this->bitsIncommingPutterHolder,
		 "incommingBitHopContextWindowProbeName",
		 numHops)->put(value);
}

void
HopContextWindowProbe::putIntoBitsAggregated(unsigned int numHops, double value)
{
	getProbe(this->bitsAggregatedProbeHolder,
		 this->bitsAggregatedPutterHolder,
		 "aggregatedBitHopContextWindowProbeName",
		 numHops)->put(value);
}

void
HopContextWindowProbe::putIntoCompoundsIncomming(unsigned int numHops, double value)
{
	getProbe(this->compoundsIncommingProbeHolder,
		 this->compoundsIncommingPutterHolder,
		 "incommingCompoundHopContextWindowProbeName",
		 numHops)->put(value);
}

void
HopContextWindowProbe::putIntoCompoundsAggregated(unsigned int numHops, double value)
{
	getProbe(this->compoundsAggregatedProbeHolder,
		 this->compoundsAggregatedPutterHolder,
		 "aggregatedCompoundHopContextWindowProbeName",
		 numHops)->put(value);
}

void
HopContextWindowProbe::periodically()
{
	if(!config_.isNone("managerName")
	{
		friends.manager->setSpecificContext();
	}

	assure(bitsIncoming.size() == compoundsIncoming.size(),
	       "Different number of different hop-counts for bitsIncoming and compoundsIncomming");
	storeProbes(bitsIncoming, bitsIncomingPutter);
	storeProbes(compoundsIncoming, compoundsIncomingOutter);

	assure(bitsAggregated.size() == compoundsAggregated.size(),
	       "Different number of different hop-counts for bitsAggregated and compoundsAggregated");
	storeProbes(bitsAggregated, bitsAggregatesPutter);
	storeProbes(compoundsAggregated, compoundsAggregatedPutter);

	// Call base class function
	this->wns::ldk::probe::Window::periodically();
}

void
HopContextWindowProbe::storeProbes(SlidingWindowMap& probeHolder, PutterMap& putterHolder)
{
	assure(probeHolder.size() == putterHolder.size(), "Number of sliding windows and putters is not equal");
	for(SlidingWindowMap::const_iterator probeItr = probeHolder.begin(); probeItr != probeHolder.end(); ++probeItr)
	{
		unsigned int numHops = itr->first;
		assure(putterHolder.knows(numHops), "putterHolder does not know " << numHops << " hops, but probeHolder does");
		putterHolder.find(numHops)->put(itr->second.getPerSecond());
	}
}
