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
 
#include <WIFIMAC/pathselection/LinkQualityMeasurement.hpp>
#include <WIFIMAC/Layer2.hpp>

#include <WNS/container/UntypedRegistry.hpp>

using namespace wifimac::pathselection;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	wifimac::pathselection::LinkQualityMeasurement,
	wns::ldk::FunctionalUnit,
	"wifimac.pathselection.LinkQualityMeasurement",
	wns::ldk::FUNConfigCreator);

LinkQuality::LinkQuality(wns::service::dll::UnicastAddress _rx,
			 wns::simulator::Time _referenceFlightTime,
			 wns::simulator::Time _periodicity,
			 LinkQualityMeasurement* _parent):
		rx(_rx),
		referenceFlightTime(_referenceFlightTime),
		periodicity(_periodicity),
		parent(_parent),
		rxLastFrame(true),
		lastRxFrameFlightTime(_referenceFlightTime),
		linkCostTx(_periodicity, true),
		oldLinkCostTxAvg(1.0),
		uniform(0.0, 1.0, wns::simulator::getRNG())
{
	this->startPeriodicTimeout(periodicity, periodicity);
	assure(referenceFlightTime > 0, "referenceFlighttime must be greater than 0");
}

void LinkQuality::receivedLinkQualityIndicator(const LinkQualityMeasurementCommand* lqm)
{
	assure(lqm->peer.srcMACAddress == rx, "Wrong lqm for this LinkQuality object");

	rxLastFrame = true;

	assure(lqm->peer.lastFrameFlightTime > 0, "received lqm with lqm->peer.lastFrameFlightTime " << lqm->peer.lastFrameFlightTime);
	linkCostTx.put(lqm->peer.lastFrameFlightTime/referenceFlightTime);
	Metric newLinkCostTxAvg = linkCostTx.getAbsolute()/linkCostTx.getNumSamples();
	//Metric newLinkCostTx = lqm->peer.lastFrameFlightTime/referenceFlightTime;

	if(!oldLinkCostTxAvg.isNear(newLinkCostTxAvg))
	{
		oldLinkCostTxAvg = newLinkCostTxAvg;
		parent->newLinkCost(rx, newLinkCostTxAvg);
	}

	lastRxFrameFlightTime = wns::simulator::getEventScheduler()->getTime() - lqm->peer.txTime;
}

void LinkQuality::periodically()
{
	assure(!hasTimeoutSet(), "new period over, but still timeout set");
	if(!rxLastFrame)
	{
		// no reply since the last transmission, link may be broken
		linkCostTx.put(oldLinkCostTxAvg.toDouble() * 100);
		oldLinkCostTxAvg = linkCostTx.getAbsolute()/linkCostTx.getNumSamples();
		parent->newLinkCost(rx, oldLinkCostTxAvg);
	}

	this->setTimeout(uniform.get() * periodicity/2);
	rxLastFrame = false; 
}

void LinkQuality::onTimeout()
{
	parent->sendLinkMeasurement(rx, lastRxFrameFlightTime);
}

wns::simulator::Time LinkQuality::getLastRxFrameFlightTime() const
{
	return(lastRxFrameFlightTime);
}

void LinkQuality::hasSendPiggybackedLQM()
{
	// delay the next periodically transmission
	if(hasTimeoutSet())
	{
		cancelTimeout();
	}
	cancelPeriodicTimeout();
	startPeriodicTimeout(periodicity, periodicity);
}

LinkQualityMeasurement::LinkQualityMeasurement(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& _config):
	wns::ldk::fu::Plain<LinkQualityMeasurement, LinkQualityMeasurementCommand>(fun),
	config(_config),
	frameSize(config.get<Bit>("frameSize")),
	referenceFlightTime(config.get<wns::simulator::Time>("referenceFlightTime")),
	period(config.get<wns::simulator::Time>("period")),
	logger(config.get("logger")),
	ucName(config.get<std::string>("upperConvergenceName")),
	doPiggybacking(config.get<bool>("doPiggybacking"))
{
	if(doPiggybacking)
	{
		piggybackingPeriod = config.get<int>("piggybackingPeriod");
		numPiggybacked = 0;
	}
	friends.uc = NULL;
}

LinkQualityMeasurement::~LinkQualityMeasurement()
{
	linkQualities.clear();
}

void LinkQualityMeasurement::onFUNCreated()
{
	// todo: actually, this should not be neccessary, as only the correct stationTypes will have this in
	// their FUN --> make an assure, not an if()
	if(getFUN()->getLayer<wifimac::Layer2*>()->getStationType() != dll::StationTypes::UT())
	{
		ps = getFUN()->getLayer<wifimac::Layer2*>()->getManagementService<wifimac::pathselection::PathSelectionInterface>
				(config.get<std::string>("pathSelectionServiceName"));
		assure(ps, "Could not get PathSelection Management Service");

		// Observer for new links
		wifimac::management::LinkNotificator* linkNotificator =
				getFUN()->findFriend<wifimac::management::LinkNotificator*>(
				config.get<std::string>("linkNotificationName"));
		this->wns::Observer<LinkNotificationInterface>::startObserving(linkNotificator);

		friends.uc = getFUN()->findFriend<dll::UpperConvergence*>(ucName);
		assure(friends.uc, "Could not get " << ucName << " from my FUN");
	}
	
	myMACAddress = getFUN()->getLayer<wifimac::Layer2*>()->getDLLAddress();
}

bool LinkQualityMeasurement::doIsAccepting(const wns::ldk::CompoundPtr& _compound) const
{
	return getConnector()->hasAcceptor(_compound);
} //doIsAccepting

void LinkQualityMeasurement::doSendData(const wns::ldk::CompoundPtr& compound)
{
	assure(compound, "doSendData called with an invalid compound.");

	// see todo in onFUNCreated()
	if((getFUN()->getLayer<wifimac::Layer2*>()->getStationType() != dll::StationTypes::UT()) && (doPiggybacking))
	{
		assure(friends.uc != NULL, "friends.uc not set");
		assure(getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), friends.uc),
			"upper convergence command is not activated");
	
		dll::UpperCommand* uc = friends.uc->getCommand(compound->getCommandPool());
		assure(uc->peer.targetMACAddress.isValid(), "uc->peer.targetMACAddress is not valid");
		
		if(linkQualities.knows(uc->peer.targetMACAddress))
		{
			numPiggybacked = (numPiggybacked+1) % piggybackingPeriod;
			if (numPiggybacked == piggybackingPeriod-1)
			{
				LinkQualityMeasurementCommand* lqm = activateCommand(compound->getCommandPool());
			
				lqm->peer.srcMACAddress = myMACAddress;
				lqm->peer.dstMACAddress = uc->peer.targetMACAddress;
				lqm->peer.txTime = wns::simulator::getEventScheduler()->getTime();
				lqm->peer.lastFrameFlightTime = linkQualities.find(uc->peer.targetMACAddress)->getLastRxFrameFlightTime();
				lqm->magic.isPiggybacked = true;
			
				MESSAGE_BEGIN(NORMAL, this->logger, m, "");
				m << ": piggyback linkMeasurement with dst " << uc->peer.targetMACAddress;
				m << ", lastRxFrameFlightTime " << linkQualities.find(uc->peer.targetMACAddress)->getLastRxFrameFlightTime();
				MESSAGE_END();
				
				linkQualities.find(uc->peer.targetMACAddress)->hasSendPiggybackedLQM();
			}
		}
	}
	
	getConnector()->getAcceptor(compound)->sendData(compound);
} // doSendData

void LinkQualityMeasurement::doWakeup()
{
	// simply forward the wakeup call
	getReceptor()->wakeup();
} // doWakeup

void LinkQualityMeasurement::doOnData(const wns::ldk::CompoundPtr& compound)
{
	assure(compound, "doOnData called with an invalid compound.");

	if(getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), this))
	{
		// this link-measurement frame is consumed here
		LinkQualityMeasurementCommand* lqm = getCommand(compound->getCommandPool());
		if(!linkQualities.knows(lqm->peer.srcMACAddress))
		{
			// this is a lqi from a very fresh link, we haven't even received at beacon by that one, but
			// the peer has received a beacon from myself. We do not hurry and wait for the beacon before doing
			// anything
		}
		else
		{
			MESSAGE_BEGIN(NORMAL, this->logger, m, "");
			m << ": received LinkMeasurement from " << lqm->peer.srcMACAddress;
			m << ", lastFrameFlightTime " << lqm->peer.lastFrameFlightTime;
			MESSAGE_END();	
			linkQualities.find(lqm->peer.srcMACAddress)->receivedLinkQualityIndicator(lqm);
		}
		
		if(lqm->magic.isPiggybacked)
		{
			assure(doPiggybacking, "received lqm-command, but piggybacking is deactivated");
			// forward frame upwards
			getDeliverer()->getAcceptor(compound)->onData(compound);
		}
	}
	else
	{
		// forward frame upwards
		getDeliverer()->getAcceptor(compound)->onData(compound);
	}
}

void LinkQualityMeasurement::sendLinkMeasurement(wns::service::dll::UnicastAddress rx, wns::simulator::Time last)
{
	wns::ldk::CompoundPtr compound(new wns::ldk::Compound(getFUN()->getProxy()->createCommandPool()));
	LinkQualityMeasurementCommand* lqm = activateCommand(compound->getCommandPool());
	
	lqm->peer.srcMACAddress = myMACAddress;
	lqm->peer.dstMACAddress = rx;
	lqm->peer.txTime = wns::simulator::getEventScheduler()->getTime();
	lqm->peer.lastFrameFlightTime = last;
	lqm->magic.isPiggybacked = false;
	
	// also activate upperCommand
	dll::UpperCommand* uc = friends.uc->activateCommand(compound->getCommandPool());
	uc->peer.sourceMACAddress = myMACAddress;
	uc->peer.targetMACAddress = rx;
	
	MESSAGE_BEGIN(NORMAL, this->logger, m, "");
	m << ": sendLinkMeasurement with dst " << rx;
	m << ", lastFrameFlightTime " << last;
	MESSAGE_END();	
	
	getConnector()->getAcceptor(compound)->sendData(compound);
}

void LinkQualityMeasurement::newLinkCost(wns::service::dll::UnicastAddress rx, Metric cost)
{
	// signal link-cost to path selection
	ps->updatePeerLink(myMACAddress, rx, cost);
}

void LinkQualityMeasurement::calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const
{
	if(getFUN()->getProxy()->commandIsActivated(commandPool, this))
	{
		if(getCommand(commandPool)->magic.isPiggybacked)
		{
			Bit magicIEsize = 8*8*3; // 8bits for IE-Hdr, Length, timestamp
			getFUN()->getProxy()->calculateSizes(commandPool, commandPoolSize, dataSize, this);
			commandPoolSize += magicIEsize;
		}
		else
		{
			// todo: shift values to PyConfig
			Bit managementMACHdrSize = 24*8;
			Bit timestampSize = 8*8;
		
			dataSize = frameSize;
			commandPoolSize = managementMACHdrSize + timestampSize;
		}
	}
	else
	{
		// get sizes from upper layers
		getFUN()->getProxy()->calculateSizes(commandPool, commandPoolSize, dataSize, this);
	}
}
	
void LinkQualityMeasurement::onLinkIndication(const wns::service::dll::UnicastAddress myself,
		      const wns::service::dll::UnicastAddress peer)
{
	if(!linkQualities.knows(peer))
	{
		// new, unknown link -> signal to path selection
		ps->createPeerLink(myself, peer, Metric(1));
	
		// create entry in map
		linkQualities.insert(peer, new LinkQuality(peer, referenceFlightTime, period, this));
	}	
}

