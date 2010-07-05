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

#include <WIFIMAC/management/Beacon.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/Layer2.hpp>
#include <WNS/service/dll/StationTypes.hpp>
#include <WNS/Exception.hpp>
#include <DLL/StationManager.hpp>

using namespace wifimac::management;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::management::Beacon,
    wns::ldk::FunctionalUnit,
    "wifimac.management.Beacon",
    wns::ldk::FUNConfigCreator);

Beacon::Beacon(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<Beacon, BeaconCommand>(fun),
    config(config_),
    logger(config.get("logger")),
    currentBeacon(),
    phyUserCommandName(config.get<std::string>("phyUserCommandName")),
    scanFrequencies(config.getSequence("myConfig.scanFrequencies")),
    beaconPhyMode(config.getView("myConfig.beaconPhyMode")),
    bssId(config.get<std::string>("myConfig.bssId")),
    beaconRxStrength(),
    bssFrequencies()
{
    friends.manager = NULL;
    MESSAGE_SINGLE(NORMAL, this->logger, "created");
}

Beacon::~Beacon()
{

}

void
Beacon::onFUNCreated()
{
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(config.get<std::string>("managerName"));
    assure(friends.manager, "Management entity not found");

    if (config.get<bool>("myConfig.enabled"))
    {
        assure(friends.manager->getStationType() != wns::service::dll::StationTypes::UT(),
               "STAs cannot transmit a beacon, as nobody can synchonize to them");
        // starting the periodic beacon transmission
        wns::simulator::Time delay = config.get<wns::simulator::Time>("myConfig.delay");
        wns::simulator::Time period = config.get<wns::simulator::Time>("myConfig.period");
        startPeriodicTimeout(period, delay);
        MESSAGE_SINGLE(NORMAL, this->logger, "Starting beacon with delay " << delay << " and period " << period);
    }

    if (friends.manager->getStationType() == wns::service::dll::StationTypes::UT())
    {
        scanFrequencies = config.getSequence("myConfig.scanFrequencies");
        freqIter = scanFrequencies.begin<double>();
        scanDuration = config.get<wns::simulator::Time>("myConfig.scanDuration");
        // set the transceiver to the first frequency
        MESSAGE_SINGLE(NORMAL, this->logger, "Set scanning frequency to " << *freqIter);
        friends.manager->getPhyUser()->setFrequency(*freqIter);
        // start scanning for the best beacon for association
        this->setTimeout(scanDuration);
    }
}

void Beacon::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    assure(friends.manager->getFrameType(compound->getCommandPool()) == BEACON, "Received frame is not a beacon");

    if (friends.manager->getTransmitterAddress(compound->getCommandPool()) == friends.manager->getMACAddress())
    {
        // Filter out own beacons
        return;
    }

    // store the received power in case of later association
    if(friends.manager->getStationType() == wns::service::dll::StationTypes::UT() and this->hasTimeoutSet())
    {

        // to strings are equal if compare returns 0
        if(this->bssId.compare(getCommand(compound->getCommandPool())->peer.bssId) == 0)
        {
            wifimac::convergence::PhyUserCommand* puc =
                getFUN()->getCommandReader(phyUserCommandName)->readCommand<wifimac::convergence::PhyUserCommand>(compound->getCommandPool());
            beaconRxStrength[puc->local.rxPower + puc->local.interference] = friends.manager->getTransmitterAddress(compound->getCommandPool());
            bssFrequencies[friends.manager->getTransmitterAddress(compound->getCommandPool())] = *freqIter;
            MESSAGE_BEGIN(NORMAL, this->logger, m, "Received Beacon from ");
            m << friends.manager->getTransmitterAddress(compound->getCommandPool());
            m << " with " << (puc->local.rxPower + puc->local.interference);
            MESSAGE_END();
        }
        else
        {
            MESSAGE_BEGIN(NORMAL, this->logger, m, "Received beacon with bssId");
            m << getCommand(compound->getCommandPool())->peer.bssId;
            m << " only associate to bssId ";
            m << this->bssId;
            MESSAGE_END();
        }
    }

    // announce the link to the observers
    // TODO: This should be the task of the LinkQualityMeasurement
    this->wns::Subject<ILinkNotification>::forEachObserver(OnLinkIndication(friends.manager->getMACAddress(),
                                                                                    friends.manager->getTransmitterAddress(compound->getCommandPool())));
}

void Beacon::processOutgoing(const wns::ldk::CompoundPtr& /*compound*/)
{
    throw wns::Exception("Impossible to call processOutgoing in Beacon FU");
}

bool Beacon::hasCapacity() const
{
    return false;
}

const wns::ldk::CompoundPtr Beacon::hasSomethingToSend() const
{
    return this->currentBeacon;
}

wns::ldk::CompoundPtr Beacon::getSomethingToSend()
{
    wns::ldk::CompoundPtr it = this->currentBeacon;
    this->currentBeacon = wns::ldk::CompoundPtr();
    return(it);
}

void
Beacon::periodically()
{
    this->currentBeacon = friends.manager->createCompound(friends.manager->getMACAddress(), wns::service::dll::UnicastAddress(), BEACON, 0.0);
    friends.manager->setPhyMode(this->currentBeacon->getCommandPool(), beaconPhyMode);
    BeaconCommand* bc = this->activateCommand(this->currentBeacon->getCommandPool());
    bc->peer.bssId = this->bssId;
    tryToSend();
}

void
Beacon::onTimeout()
{
    assure(friends.manager->getStationType() == wns::service::dll::StationTypes::UT(), "Only STAs (UTs) can become associated");

    MESSAGE_SINGLE(NORMAL, this->logger, "Timeout for beacon scanning on frequency " << *freqIter);
    ++freqIter;

    if(freqIter != scanFrequencies.end<double>())
    {
        // set the transceiver to the next frequency
        MESSAGE_SINGLE(NORMAL, this->logger, "Set scanning frequency to " << *freqIter);
        friends.manager->getPhyUser()->setFrequency(*freqIter);
        // start next round of scanning for the best beacon for association
        this->setTimeout(scanDuration);

        return;
    }

    if(beaconRxStrength.empty())
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "No beacons received, start scanning again");
        freqIter = scanFrequencies.begin<double>();
        friends.manager->getPhyUser()->setFrequency(*freqIter);
        this->setTimeout(scanDuration);
    }
    else
    {
        {
            MESSAGE_BEGIN(NORMAL, this->logger, m, "Start association, received beacons: ");
            for (std::map<wns::Power, wns::service::dll::UnicastAddress>::reverse_iterator itr = beaconRxStrength.rbegin();
                 itr != beaconRxStrength.rend();
                 ++itr)
            {
                m << "(" << itr->first << " from " << itr->second << " on " << bssFrequencies[itr->second] << "MHz)";
            }
            MESSAGE_END();
        }

        std::map<wns::Power, wns::service::dll::UnicastAddress>::reverse_iterator itr = beaconRxStrength.rbegin();
        friends.manager->getPhyUser()->setFrequency(bssFrequencies[itr->second]);
        friends.manager->associateWith(itr->second);

        this->getFUN()->getLayer<wifimac::Layer2*>()->updateContext
            ("MAC.STAAssociatedToAP",
             getFUN()->getLayer<wifimac::Layer2*>()->getStationManager()->getStationByMAC(itr->second)->getID());
    }
}

void
Beacon::calculateSizes(const wns::ldk::CommandPool* /*commandPool*/, Bit& commandPoolSize, Bit& sduSize) const
{
    //No upper FUs, no upper sizes
    //this->getFUN()->calculateSizes(commandPool, commandPoolSize, sduSize, this);

    // todo: shift values to PyConfig
    Bit managementMACHdrSize = 24*8;
    Bit timestampSize = 8*8;
    Bit beaconIntervalSize = 2*8;
    Bit capabilityInformationSize = 2*8;
    Bit ssidSize = 2*8;
    Bit supportedRatesSize = 3*8;
    Bit timSize = 6*8;

    commandPoolSize = managementMACHdrSize + timestampSize + beaconIntervalSize + capabilityInformationSize + ssidSize + supportedRatesSize + timSize;
    sduSize = 0;
} // calculateSizes
