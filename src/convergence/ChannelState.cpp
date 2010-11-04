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

#include <WIFIMAC/convergence/ChannelState.hpp>
#include <WIFIMAC/lowerMAC/NewRTSCTS.hpp>

#include <WNS/probe/bus/utils.hpp>

using namespace wifimac::convergence;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::convergence::ChannelState,
    wns::ldk::FunctionalUnit,
    "wifimac.convergence.ChannelState",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::convergence::ChannelState,
    wns::ldk::probe::Probe,
    "wifimac.convergence.ChannelState",
    wns::ldk::FUNConfigCreator);

ChannelState::ChannelState(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<ChannelState, ChannelStateCommand>(fun),
    wns::ldk::probe::Probe(),
    wns::events::PeriodicTimeout(),

    config(config_),
    logger(config.get("logger")),

    // thresholds
    rawEnergyThreshold(config.get<wns::Power>("myConfig.rawEnergyThreshold")),
    phyCarrierSenseThreshold(config.get<wns::Power>("myConfig.phyCarrierSenseThreshold")),

    // rememberinf lastCS
    lastCS(idle),
    latestNAV(0),
    waitForReply(false),

    // FU and command names, durations
    managerName(config.get<std::string>("managerName")),
    phyUserCommandName(config.get<std::string>("phyUserCommandName")),
    rtsctsCommandName(config.get<std::string>("rtsctsCommandName")),
    txStartEndName(config.get<std::string>("txStartEndName")),
    rxStartEndName(config.get<std::string>("rxStartEndName")),

    sifsDuration(config_.get<wns::simulator::Time>("myConfig.sifsDuration")),

    // probing the channel busy fraction
    channelBusyFractionProbe(),
    channelBusyFractionMeasurementPeriod(config.get<wns::simulator::Time>("myConfig.channelBusyFractionMeasurementPeriod")),
    channelBusyTime(0),
    channelBusySlotStart(0),
    channelBusyLastChangeToBusy(0)
{
    // configure the active indicators
    activeIndicators.rawEnergyDetection = config.get<bool>("myConfig.useRawEnergyDetection");
    activeIndicators.phyCarrierSense = config.get<bool>("myConfig.usePhyCarrierSense");
    activeIndicators.ownRx = config.get<bool>("myConfig.useOwnRx");
    activeIndicators.nav = config.get<bool>("myConfig.useNAV");
    activeIndicators.ownTx = config.get<bool>("myConfig.useOwnTx");

    // reset all indicator values to zero
    indicators.rawEnergy = wns::Power::from_mW(0);
    indicators.phyCarrierSense = wns::Power::from_mW(0);
    indicators.ownTx = false;
    indicators.ownRx = false;

    assure(!activeIndicators.rawEnergyDetection, "ChannelState based on rawEnergyDetection not yet implemented!");

    MESSAGE_BEGIN(NORMAL, logger, m, "Created, indicators: ");
    if (activeIndicators.rawEnergyDetection)  m << "rawEnergyDetection(" << rawEnergyThreshold << ") ";
    if (activeIndicators.phyCarrierSense)     m << "phyCarrierSense(" << phyCarrierSenseThreshold << ") ";
    if (activeIndicators.nav)                 m << "NAV ";
    if (activeIndicators.ownTx)               m << "ownTx ";
    if (activeIndicators.ownRx)               m << "ownRx";
    MESSAGE_END();

    // Probe-Stuff
    // read the localContext from the config
    wns::probe::bus::ContextProviderCollection localContext(&fun->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config.len("localIDs.keys()"); ++ii)
    {
        std::string key = config.get<std::string>("localIDs.keys()",ii);
        unsigned long int value  = config.get<unsigned long int>("localIDs.values()",ii);
        localContext.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
        MESSAGE_SINGLE(VERBOSE, logger, "Using Local IDName '"<<key<<"' with value: "<<value);
    }
    this->channelBusyFractionProbe = wns::probe::bus::collector(localContext, config, "busyFractionProbeName");
    this->startPeriodicTimeout(this->channelBusyFractionMeasurementPeriod);

    friends.manager = NULL;
}

ChannelState::~ChannelState()
{
}

void ChannelState::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);

    // Observe txStartEnd
    if(activeIndicators.ownTx)
    {
        this->wns::Observer<wifimac::convergence::ITxStartEnd>::startObserving(getFUN()->findFriend<wifimac::convergence::TxStartEndNotification*>(txStartEndName));
    }

    // Observe rxStartEnd
    if(activeIndicators.ownRx)
    {
        this->wns::Observer<wifimac::convergence::IRxStartEnd>::startObserving(getFUN()->findFriend<wifimac::convergence::RxStartEndNotification*>(rxStartEndName));
    }
}

void ChannelState::setCarrierSensingService(wns::service::Service* cs)
{
    assure(cs, "must be non-NULL");
    assureType(cs, wns::service::phy::ofdma::Notification*);
    this->myCS = dynamic_cast<wns::service::phy::ofdma::Notification*>(cs);
    this->myCS->registerRSSHandler(this);

} // setNotificationService

void
ChannelState::onTxStart(const wns::ldk::CompoundPtr& /*compound*/)
{
    this->indicators.ownTx = true;
    MESSAGE_SINGLE(NORMAL, logger, "Start of own transmission --> busy");
    checkNewCS();
}

void
ChannelState::onTxEnd(const wns::ldk::CompoundPtr& compound)
{
    this->indicators.ownTx = false;

    if((friends.manager->getFrameExchangeDuration(compound->getCommandPool()) > this->sifsDuration) or
       (friends.manager->getReplyTimeout(compound->getCommandPool()) > 0.0))
    {
        MESSAGE_SINGLE(NORMAL, logger, "End of own transmission, awaiting reply --> set NAV");
        // something expected after the frame (either own tx or reply) --> set
        // short NAV

        wns::simulator::Time duration = friends.manager->getReplyTimeout(compound->getCommandPool());
        if(wns::simulator::getEventScheduler()->getTime() + duration > this->latestNAV)
        {
            latestNAV = wns::simulator::getEventScheduler()->getTime() + duration;
            this->setNewTimeout(duration);
            this->waitForReply = true;
        }
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, logger, "End of own transmission --> idle");
    }
    checkNewCS();
}

void
ChannelState::onRxStart(const wns::simulator::Time /*expRxDuration*/)
{
    this->indicators.ownRx = true;
    checkNewCS();
}

void
ChannelState::onRxEnd()
{
    this->indicators.ownRx = false;
    checkNewCS();
}

void
ChannelState::onRxError()
{
    this->onRxEnd();
}

void
ChannelState::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    // The onTxStart/onTxEnd is not issued for preambles!
    if(friends.manager->getFrameType(compound->getCommandPool()) == PREAMBLE)
    {
        this->onTxStart(compound);
    }
}

void
ChannelState::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    assure(compound, "doOnData called with an invalid compound.");
    // Derive channel state from NAV information send in the MAC header
    if(friends.manager->getFrameType(compound->getCommandPool()) != PREAMBLE)
    {
        if(friends.manager->isForMe(compound->getCommandPool()))
        {

            if(this->waitForReply)
            {
                // abort own NAV
                this->latestNAV = wns::simulator::getEventScheduler()->getTime();
                cancelTimeout();
                this->waitForReply = false;
                checkNewCS();
            }
            // NAV is only set for frames NOT for me
            return;
        }


        wns::simulator::Time duration = 0;
        if(isRTS(compound))
        {
            assure(friends.manager->getFrameExchangeDuration(compound->getCommandPool()) > 0,
                   "Received RTS with FrameExchangeDuration <= 0");

            // special handling of RTS frames: set duration only until expected
            // reception start of the data frame:
            duration = friends.manager->getReplyTimeout(compound->getCommandPool());
        }
        else
        {
            duration = friends.manager->getFrameExchangeDuration(compound->getCommandPool());
            MESSAGE_SINGLE(NORMAL, logger, "Received valid compound with fExDur " << duration);

            if(duration < this->sifsDuration)
            {
                return;
            }

        }

        if(wns::simulator::getEventScheduler()->getTime() + duration > this->latestNAV)
        {
            latestNAV = wns::simulator::getEventScheduler()->getTime() + duration;

            // signal NAV
            this->wns::Subject<INetworkAllocationVector>::forEachObserver
                (OnChangedNAV(true, friends.manager->getTransmitterAddress(compound->getCommandPool())));

            this->setNewTimeout(duration);
            this->checkNewCS();
        }

    }
    else
    {
        wns::simulator::Time duration = friends.manager->getFrameExchangeDuration(compound->getCommandPool());
        MESSAGE_SINGLE(NORMAL, logger, "Received valid preamble with fExDur " << duration);
        if(wns::simulator::getEventScheduler()->getTime() + duration > this->latestNAV)
        {
            latestNAV = wns::simulator::getEventScheduler()->getTime() + duration;
            this->setNewTimeout(duration);
            this->checkNewCS();
        }
    }

}

bool ChannelState::isRTS(const wns::ldk::CompoundPtr& compound) const
{
    return((getFUN()->getCommandReader(rtsctsCommandName)->commandIsActivated(compound->getCommandPool())) and
           (getFUN()->getCommandReader(rtsctsCommandName)->readCommand<wifimac::lowerMAC::RTSProviderCommand>(compound->getCommandPool())->isRTS()));
}

void ChannelState::onTimeout()
{
    // NAV has ended
    assure(latestNAV == wns::simulator::getEventScheduler()->getTime(), "indicators.nav has different time than now!");
    MESSAGE_SINGLE(NORMAL, logger, "NAV has finished");
    if(this->waitForReply)
    {
        // was own NAV
        this->waitForReply = false;
    }
    this->wns::Subject<INetworkAllocationVector>::forEachObserver
        (OnChangedNAV(false, wns::service::dll::UnicastAddress()));

    myCS->updateRequest();

    this->checkNewCS();
}

void ChannelState::periodically()
{
    // pre-tick: compute busy of the current slot
    if(this->getCurrentChannelState() == busy)
    {
        this->probeChannelIdle();
        this->probeChannelBusy();
    }
    double busyFraction = this->channelBusyTime / this->channelBusyFractionMeasurementPeriod;

    // tick -> put probe
    this->channelBusyFractionProbe->put(busyFraction);

    this->channelBusySlotStart = wns::simulator::getEventScheduler()->getTime();
    this->channelBusyTime = 0.0;
}

void ChannelState::onRSSChange(wns::Power newRSS)
{
    MESSAGE_SINGLE(NORMAL, logger, "RSS changed to " << newRSS);

    if (activeIndicators.phyCarrierSense)
    {
        indicators.phyCarrierSense = newRSS;
        checkNewCS();
    }

    for(int i=0; i < rssObservers.size(); i++)
    {
        rssObservers[i]->onRSSChange(newRSS);
    }
}

CS ChannelState::getCurrentChannelState() const
{
    // the channel state is a big OR over the active indicators

    if ((activeIndicators.ownTx) and (indicators.ownTx))
    {
        MESSAGE_SINGLE(VERBOSE, logger, "ownTx is active");
        return(busy);
    }

    if((activeIndicators.ownRx) and (indicators.ownRx))
    {
        MESSAGE_SINGLE(VERBOSE, logger, "ownRx is active");
        return(busy);
    }

    if ((activeIndicators.rawEnergyDetection) and (indicators.rawEnergy > rawEnergyThreshold))
    {
        MESSAGE_SINGLE(VERBOSE, logger, "rawEnergy " << indicators.rawEnergy << " > "  <<  rawEnergyThreshold);
        return(busy);
    }

    if ((activeIndicators.phyCarrierSense) and (indicators.phyCarrierSense > phyCarrierSenseThreshold))
    {
        MESSAGE_SINGLE(VERBOSE, logger, "phyCS " << indicators.phyCarrierSense << " > "  <<  phyCarrierSenseThreshold);
        return(busy);
    }

    if((activeIndicators.nav) and (this->hasTimeoutSet()))
    {
        MESSAGE_SINGLE(VERBOSE, logger, "NAV is set until " << this->latestNAV);
        return(busy);
    }

    return(idle);
}

void ChannelState::checkNewCS()
{
    CS newCS = getCurrentChannelState();

    if (newCS != lastCS)
    {
        // Channel has changed, inform observers
        MESSAGE_SINGLE(NORMAL, logger, "Channel changed to " << ((newCS == busy)? "busy":"idle"));
        this->wns::Subject<IChannelState>::forEachObserver(OnChangedCS(newCS));
        lastCS = newCS;

        if(newCS == busy)
        {
            this->probeChannelBusy();
        }
        else
        {
            this->probeChannelIdle();
        }
    }
}

void ChannelState::probeChannelBusy()
{
    this->channelBusyLastChangeToBusy = wns::simulator::getEventScheduler()->getTime();
}

void ChannelState::probeChannelIdle()
{
    if(this->channelBusyLastChangeToBusy > this->channelBusySlotStart)
    {
        // latest change to channel busy occured during this slot
        this->channelBusyTime += wns::simulator::getEventScheduler()->getTime()
            - this->channelBusyLastChangeToBusy;
    }
    else
    {
        // lastest change to busy occured before this slot
        this->channelBusyTime += wns::simulator::getEventScheduler()->getTime()
            - this->channelBusySlotStart;
    }
    assure(this->channelBusyTime <= this->channelBusyFractionMeasurementPeriod+1e9,
           "busyTime must be leq than measurementPeriod");
}
