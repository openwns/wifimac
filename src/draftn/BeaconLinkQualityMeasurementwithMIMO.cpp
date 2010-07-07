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

#include <WIFIMAC/draftn/BeaconLinkQualityMeasurementwithMIMO.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/FrameType.hpp>

#include <DLL/Layer2.hpp>
#include <WNS/service/dll/StationTypes.hpp>
#include <WNS/probe/bus/utils.hpp>

#include <math.h>

using namespace wifimac::draftn;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::BeaconLinkQualityMeasurementwithMIMO,
    wns::ldk::FunctionalUnit,
    "wifimac.draftn.BeaconLinkQualityMeasurementwithMIMO",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::BeaconLinkQualityMeasurementwithMIMO,
    wns::ldk::probe::Probe,
    "wifimac.draftn.BeaconLinkQualityMeasurementwithMIMO",
    wns::ldk::FUNConfigCreator);

BroadcastLinkQualitywithMIMO::BroadcastLinkQualitywithMIMO(const wns::pyconfig::View& config_,
                                           BeaconLinkQualityMeasurementwithMIMO* parent_,
                                           wns::service::dll::UnicastAddress peerAddress_,
                                           wns::service::dll::UnicastAddress myAddress_,
                                           wns::simulator::Time interval_):
    config(config_),
    parent(parent_),
    peerAddress(peerAddress_),
    myAddress(myAddress_),
    successRate(interval_*config.get<int>("myConfig.windowLength"), true),
    missedBeaconsInRow(10),
    linkCreated(false),
    meanFrameSize(config.get<Bit>("myConfig.meanFrameSize")),
    maximumACKDuration(config.get<wns::simulator::Time>("myConfig.maximumACKDuration")),
    slotDuration(config.get<wns::simulator::Time>("myConfig.slotDuration")),
    sifsDuration(config.get<wns::simulator::Time>("myConfig.sifsDuration")),
    preambleDuration(config.get<wns::simulator::Time>("myConfig.preambleDuration")),
    scalingFactor(config.get<double>("myConfig.scalingFactor")),
    maxMissedBeacons(config.get<int>("myConfig.maxMissedBeacons"))
{
    ps = parent->getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::pathselection::IPathSelection>
        (config.get<std::string>("pathSelectionServiceName"));
    sinrMIB = parent->getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::draftn::SINRwithMIMOInformationBase>
        (config.get<std::string>("sinrMIBServiceName"));
    MESSAGE_SINGLE(VERBOSE, parent->logger, "BroadcastLinkQualitywithMIMO for "<< peerAddress << " created.");
}

double
BroadcastLinkQualitywithMIMO::getSuccessRate()
{
    MESSAGE_SINGLE(NORMAL, parent->logger, "BroadcastLinkQualitywithMIMO for " << peerAddress << ": getSuccessRate with "<< successRate.getNumSamples() << " numSamples.");

    if((successRate.getNumSamples() > 0) && (this->isActive()))
    {
        return(successRate.getAbsolute() / successRate.getNumSamples());
    }
    else
    {
        return(0.0);
    }
}

wns::Ratio
BroadcastLinkQualitywithMIMO::getAverageSINR()
{
    if(sinrMIB->knowsMeasuredSINR(peerAddress))
    {
        return(sinrMIB->getMeasuredSINR(peerAddress));
    }
    else
    {
        return(wns::Ratio::from_dB(0.0));
    }
}

SINRwithMIMOInformationBase::NumSSToFactorMap
BroadcastLinkQualitywithMIMO::getMIMOfactors()
{
    if(sinrMIB->knowsMeasuredFactor(peerAddress,1))
    {
        return(sinrMIB->getAllMeasuredFactors(peerAddress));
    }
    else
    {
        return SINRwithMIMOInformationBase::NumSSToFactorMap();
    }
}

void
BroadcastLinkQualitywithMIMO::newBeacon(wns::simulator::Time interval)
{
    // we give 1.5 x interval for the upcomming beacon
    this->curInterval = interval;
    setNewTimeout(curInterval*1.5);

    successRate.put(1.0);

    this->missedBeaconsInRow = 0;
}

wifimac::pathselection::Metric
BroadcastLinkQualitywithMIMO::newPeerMeasurement(double peerSuccessRate, wns::Ratio peerSINR,SINRwithMIMOInformationBase::NumSSToFactorMap factors)
{
    sinrMIB->putPeerSINR(peerAddress, peerSINR);
    for (SINRwithMIMOInformationBase::NumSSToFactorMap::const_iterator it=factors.begin(); it != factors.end();it++)   
	sinrMIB->putPeerFactor(peerAddress, it->second);

    // compute and send new metric for link me->peer to ps, TODO: for all numSS and factors, find best like SINRwithMIMO

    // Convert optSINR to frametime + PLCP preamble + ACK + SIFS
    // TODO: This should be a function provided by the protocol calculator
    double frameTxTime = (this->meanFrameSize)/getBestTotalRate(peerSINR,factors) + this->preambleDuration + this->sifsDuration + this->maximumACKDuration;

    // Convert rate to expected transmission time
    double ett = (1.0/peerSuccessRate) * frameTxTime + (pow(2,3+1.0/peerSuccessRate) - 1)/2*this->slotDuration;

    // scale metric with usual BPSK1/2 transmission time
    wifimac::pathselection::Metric m = ett/this->scalingFactor;

    MESSAGE_SINGLE(NORMAL, parent->logger, "BroadcastLinkQualitywithMIMO for " << peerAddress << ": new PeerMeasurement with " << peerSuccessRate << "/" << peerSINR << " --> " << m);

    if(linkCreated)
    {
        ps->updatePeerLink(myAddress, peerAddress, m);
    }
    else
    {
        ps->createPeerLink(myAddress, peerAddress, m);
        linkCreated = true;
    }

    if (ps->getProxyFor(parent->getFUN()->getLayer<dll::Layer2*>()->getDLLAddress()) == wns::service::dll::UnicastAddress())
    {

        MESSAGE_SINGLE(NORMAL, parent->logger,"Registering mesh point as proxy for itself at VPS");
        ps->registerProxy(parent->getFUN()->getLayer<dll::Layer2*>()->getDLLAddress(),parent->getFUN()->getLayer<dll::Layer2*>()->getDLLAddress());
    }
    return(m);
}

double
BroadcastLinkQualitywithMIMO::getBestRate(wns::Ratio sinr)
{
    if(sinr >= wns::Ratio::from_dB(26.5))
    {
        return (65e6);
    }
    if(sinr >= wns::Ratio::from_dB(24.8))
    {
        return (58.5e6);
    }
    if(sinr >=  wns::Ratio::from_dB(23.5))
    {
        return(48e6);
    }
    if(sinr >= wns::Ratio::from_dB(18.8))
    {
        return(39e6);
    }
    if(sinr >= wns::Ratio::from_dB(15.4))
    {
        return(26e6);
    }
    if(sinr >= wns::Ratio::from_dB(12.0))
    {
        return(19.5e6);
    }
    if(sinr >= wns::Ratio::from_dB(8.8))
    {
        return(13e6);
    }
    if(sinr >= wns::Ratio::from_dB(6.0))
    {
        return(6.5e6);
    }
    return(1);
}

double BroadcastLinkQualitywithMIMO::getBestTotalRate(wns::Ratio peerSINR, SINRwithMIMOInformationBase::NumSSToFactorMap factors)
{
  double maxBitRate = 0;
  double rate;
  for (SINRwithMIMOInformationBase::NumSSToFactorMap::const_iterator it=factors.begin(); it != factors.end();it++)   
  {
        rate = 0;
	for (int i=0; i < it->second.size();i++)
	{
	 rate += getBestRate(peerSINR+it->second[i]);	
	}
	maxBitRate = std::max(maxBitRate,rate);
  }	
  return(maxBitRate);
}

void
BroadcastLinkQualitywithMIMO::deadPeerMeasurement()
{
    // my beacon was not received by peer for several times or I did not receive
    // a beacon for several times -> either way, the link seems to be closed
    MESSAGE_SINGLE(NORMAL, parent->logger, "BroadcastLinkQualitywithMIMO for " << peerAddress << ": dead PeerMeasurement");
    ++missedBeaconsInRow;

    if(linkCreated && (!this->isActive()))
    {
        ps->closePeerLink(myAddress, peerAddress);
        linkCreated = false;
    }
}

void
BroadcastLinkQualitywithMIMO::onTimeout()
{
    this->deadPeerMeasurement();

    if(this->isActive())
    {
        // failed beacon reception -> unstable link!
        setTimeout(curInterval);

        successRate.put(0.0);
        // the averagePower only counts for successfull receptions

        ++missedBeaconsInRow;
    }

    MESSAGE_SINGLE(NORMAL, parent->logger, "BroadcastLinkQualitywithMIMO for " << peerAddress << ": timeout for new beacon, now " << missedBeaconsInRow << " missed");
}

bool
BroadcastLinkQualitywithMIMO::isActive() const
{
    return(missedBeaconsInRow <= this->maxMissedBeacons);
}

BeaconLinkQualityMeasurementwithMIMO::BeaconLinkQualityMeasurementwithMIMO(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& _config):
    wns::ldk::fu::Plain<BeaconLinkQualityMeasurementwithMIMO, BeaconLinkQualityMeasurementwithMIMOCommand>(fun),
    logger(_config.get("logger")),
    config(_config),
    beaconInterval(config.get<wns::simulator::Time>("beaconInterval")),
    phyUserCommandName(config.get<std::string>("phyUserCommandName"))
{
    friends.manager = NULL;

    // read the local IDs from the config
    wns::probe::bus::ContextProviderCollection registry(&fun->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config.len("localIDs.keys()"); ++ii)
    {
        std::string key = config.get<std::string>("localIDs.keys()",ii);
        unsigned long int value  = config.get<unsigned long int>("localIDs.values()",ii);
        registry.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
        MESSAGE_SINGLE(VERBOSE, logger, "Using Local IDName '"<<key<<"' with value: "<<value);
    }

    receivedPower = wns::probe::bus::collector(registry, config, "receivedPowerProbeName");
    peerMeasurement = wns::probe::bus::collector(registry, config, "peerMeasurementProbeName");
    linkCost = wns::probe::bus::collector(registry, config, "linkCostProbeName");
}

BeaconLinkQualityMeasurementwithMIMO::~BeaconLinkQualityMeasurementwithMIMO()
{
    linkQualities.clear();
}

void
BeaconLinkQualityMeasurementwithMIMO::onFUNCreated()
{
    friends.manager =  getFUN()->findFriend<wifimac::lowerMAC::Manager*>(config.get<std::string>("managerName"));
    assure(friends.manager, "Management entity not found");

    if(friends.manager->getStationType() != wns::service::dll::StationTypes::UT())
    {
        ps = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::pathselection::IPathSelection>
            (config.get<std::string>("pathSelectionServiceName"));
        assure(ps, "Could not get PathSelection Management Service");
    }

    myMACAddress = friends.manager->getMACAddress();
}

bool
BeaconLinkQualityMeasurementwithMIMO::doIsAccepting(const wns::ldk::CompoundPtr& _compound) const
{
    return getConnector()->hasAcceptor(_compound);
} //doIsAccepting

void
BeaconLinkQualityMeasurementwithMIMO::doSendData(const wns::ldk::CompoundPtr& compound)
{
    assure(compound, "doSendData called with an invalid compound.");
    assure(friends.manager->getStationType() != wns::service::dll::StationTypes::UT(), "Only non-UT are allowed to send beacons");
    assure(friends.manager->getFrameType(compound->getCommandPool()) == BEACON,
           "Frame type is no beacon, but " << friends.manager->getFrameType(compound->getCommandPool()));

    BeaconLinkQualityMeasurementwithMIMOCommand* blqm = activateCommand(compound->getCommandPool());
    blqm->peer.interval = beaconInterval;
    blqm->peer.rxBeaconSuccessRates = Address2SuccessRateMap();
    blqm->peer.rxBeaconSINRs = Address2RatioMap();
    blqm->peer.rxMIMOfactors = SINRwithMIMOInformationBase::AddressToFactorsMap();

    // add the results for each linkQuality to the compound
    for(adr2qualityMap::const_iterator itr = linkQualities.begin(); itr != linkQualities.end(); ++itr)
    {
        if(itr->second->isActive())
        {
            blqm->peer.rxBeaconSuccessRates.insert(itr->first, itr->second->getSuccessRate());
            blqm->peer.rxBeaconSINRs.insert(itr->first, itr->second->getAverageSINR());
            blqm->peer.rxMIMOfactors.insert(itr->first, new SINRwithMIMOInformationBase::NumSSToFactorMap(itr->second->getMIMOfactors()));
            MESSAGE_BEGIN(NORMAL, this->logger, m, "");
            m << "Added linkQualityInformation for " << itr->first;
            m << " with rate " << itr->second->getSuccessRate();
            m << ", sinr " << itr->second->getAverageSINR() << "(MIMO factors not shown)";
            MESSAGE_END();
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "No lq for " << itr->first << ", because link is currently not active");
        }
    }

    getConnector()->getAcceptor(compound)->sendData(compound);
} // doSendData

void
BeaconLinkQualityMeasurementwithMIMO::doWakeup()
{
    // simply forward the wakeup call
    getReceptor()->wakeup();
} // doWakeup

void
BeaconLinkQualityMeasurementwithMIMO::doOnData(const wns::ldk::CompoundPtr& compound)
{
    assure(compound, "doOnData called with an invalid compound.");

    if(friends.manager->getStationType() == wns::service::dll::StationTypes::UT())
    {
        // UT->nothing to do
        getDeliverer()->getAcceptor(compound)->onData(compound);
        return;
    }

    if(getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), this))
    {
        // this link-measurement frame is consumed here
        BeaconLinkQualityMeasurementwithMIMOCommand* blqm = getCommand(compound->getCommandPool());
        wifimac::convergence::PhyUserCommand* puc =
            getFUN()->getCommandReader(phyUserCommandName)->readCommand<wifimac::convergence::PhyUserCommand>(compound->getCommandPool());

        if(!linkQualities.knows(friends.manager->getTransmitterAddress(compound->getCommandPool())))
        {
            // received a beacon from a peer which was never seen before
            // create entry in map
            MESSAGE_BEGIN(NORMAL, this->logger, m, "Received first beacon from ");
            m << friends.manager->getTransmitterAddress(compound->getCommandPool());
            m << ", creating lq with interval " << blqm->peer.interval;
            MESSAGE_END();
            linkQualities.insert(friends.manager->getTransmitterAddress(compound->getCommandPool()),
                                 new BroadcastLinkQualitywithMIMO(config, this,
                                                          friends.manager->getTransmitterAddress(compound->getCommandPool()),
                                                          friends.manager->getMACAddress(),
                                                          blqm->peer.interval));
        }

        BroadcastLinkQualitywithMIMO* currentLQ = linkQualities.find(friends.manager->getTransmitterAddress(compound->getCommandPool()));
        currentLQ->newBeacon(blqm->peer.interval);

        // store the probe for received power
        receivedPower->put(compound, puc->local.rxPower.get_dBm());

        if(blqm->peer.rxBeaconSuccessRates.knows(myMACAddress))
        {
            assure(blqm->peer.rxBeaconSINRs.knows(myMACAddress),
                   "Beacon from " << friends.manager->getTransmitterAddress(compound->getCommandPool()) << " knows me in successRates, but not SINR");
            MESSAGE_SINGLE(NORMAL, this->logger, "Received beacon from "<< friends.manager->getTransmitterAddress(compound->getCommandPool()) << ", which contains new lq information");

            wifimac::pathselection::Metric m;
            m = currentLQ->newPeerMeasurement(blqm->peer.rxBeaconSuccessRates.find(myMACAddress),
                                              blqm->peer.rxBeaconSINRs.find(myMACAddress),
					      (*blqm->peer.rxMIMOfactors.find(myMACAddress)));

            // put new link cost in probe
            linkCost->put(compound, m.toDouble());
            // put positive measurement in probe
            peerMeasurement->put(compound, 1);
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "Received beacon from "<< friends.manager->getTransmitterAddress(compound->getCommandPool()) << ", but no lq information for me");
            currentLQ->deadPeerMeasurement();

            // put negative measurement in probe
            peerMeasurement->put(compound, 0);
        }
    }

    // forward frame upwards
    getDeliverer()->getAcceptor(compound)->onData(compound);
}

void BeaconLinkQualityMeasurementwithMIMO::newLinkCost(wns::service::dll::UnicastAddress rx, wifimac::pathselection::Metric cost)
{
    // signal link-cost to path selection
    ps->updatePeerLink(myMACAddress, rx, cost);
}

void BeaconLinkQualityMeasurementwithMIMO::calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const
{
    // get sizes from upper layers
    getFUN()->getProxy()->calculateSizes(commandPool, commandPoolSize, dataSize, this);

    if(getFUN()->getProxy()->commandIsActivated(commandPool, this))
    {
        Bit ieHeaderSize = 8*3; // 8bits for IE-Hdr, Length, timestamp
        Bit nextPlannedTxSize = 8*4;

        size_t numAddr = getCommand(commandPool)->peer.rxBeaconSuccessRates.size();
        assure(numAddr == getCommand(commandPool)->peer.rxBeaconSINRs.size(), "successRates and powers have different size");
        Bit lqInformationSize = (8*4 + 8 + 8) * numAddr;

        commandPoolSize = commandPoolSize + ieHeaderSize + nextPlannedTxSize + lqInformationSize;
    }
}
