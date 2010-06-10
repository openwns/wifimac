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

#include <WIFIMAC/convergence/NetworkStateProbe.hpp>
#include <WIFIMAC/convergence/TxDurationSetter.hpp>

#include <WNS/probe/bus/utils.hpp>

using namespace wifimac::convergence;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::convergence::NetworkStateProbe,
    wns::ldk::FunctionalUnit,
    "wifimac.convergence.NetworkStateProbe",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::convergence::NetworkStateProbe,
    wns::ldk::probe::Probe,
    "wifimac.convergence.NetworkStateProbe",
    wns::ldk::FUNConfigCreator);

NetworkStateProbe::NetworkStateProbe(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config) :
    wns::ldk::fu::Plain<NetworkStateProbe, NetworkStateProbeCommand>(fun),
    wns::ldk::probe::Probe(),

    logger(config.get("logger")),
    txDurationProviderCommandName(config.get<std::string>("txDurationProviderCommandName")),
    localNetworkState(),
    isTx(),
    curTxCompound(),
    curFrameTxDuration(0.0)
{
    // read the localContext from the config
    wns::probe::bus::ContextProviderCollection localContext(&fun->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config.len("localIDs.keys()"); ++ii)
    {
        std::string key = config.get<std::string>("localIDs.keys()",ii);
        unsigned long int value  = config.get<unsigned long int>("localIDs.values()",ii);
        localContext.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
        MESSAGE_SINGLE(VERBOSE, logger, "Using Local IDName '"<<key<<"' with value: "<<value);
    }

    // Add context to distinguish tx and rx
    isTx = new wns::probe::bus::contextprovider::Variable("MAC.isOwnTransmission", 0);
    localContext.addProvider(wns::probe::bus::contextprovider::Container(isTx));

    this->localNetworkState = wns::probe::bus::collector(localContext, config, "localNetworkStateProbeName");
}

NetworkStateProbe::~NetworkStateProbe()
{
    delete isTx;
}

void
NetworkStateProbe::processOutgoing(const  wns::ldk::CompoundPtr& compound)
{
    if(hasTimeoutSet())
    {
        cancelTimeout();
        onTimeout();
    }

    curTxCompound = compound;
    curFrameTxDuration = getFUN()->getCommandReader(txDurationProviderCommandName)->
        readCommand<wifimac::convergence::TxDurationProviderCommand>(compound->getCommandPool())->getDuration();

    setTimeout(curFrameTxDuration);
}

void
NetworkStateProbe::onTimeout()
{
    isTx->set(1);
    this->localNetworkState->put(curTxCompound, curFrameTxDuration);

    curTxCompound = wns::ldk::CompoundPtr();
    curFrameTxDuration = 0.0;
}


void
NetworkStateProbe::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    wns::simulator::Time frameTxDuration = getFUN()->getCommandReader(txDurationProviderCommandName)->
        readCommand<wifimac::convergence::TxDurationProviderCommand>(compound->getCommandPool())->getDuration();

    isTx->set(0);
    this->localNetworkState->put(compound, frameTxDuration);
}

