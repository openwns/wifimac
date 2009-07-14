/******************************************************************************
 * WiFiMAC (IEEE 802.11 and its alphabet soup)                                *
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

#include <WIFIMAC/pathselection/IPathSelection.hpp>
#include <WIFIMAC/pathselection/Metric.hpp>
#include <WIFIMAC/helper/contextprovider/CommandInformation.hpp>
#include <WIFIMAC/helper/contextprovider/CompoundSize.hpp>
#include <WNS/service/dll/StationTypes.hpp>
#include <WNS/Assure.hpp>
#include <DLL/StationManager.hpp>

using namespace wifimac;

STATIC_FACTORY_REGISTER_WITH_CREATOR(wifimac::Layer2,
                                     wns::node::component::Interface,
                                     "wifimac.Layer2",
                                     wns::node::component::ConfigCreator);

Layer2::Layer2(wns::node::Interface* _node, const wns::pyconfig::View& _config) :
    dll::Layer2(_node, _config, NULL),
    logger_(config.get("logger")),
    managers_()
{
    MESSAGE_SINGLE(NORMAL, logger_, "creating station" << _node->getName() << " with ID " << this->stationID << " and type " << type);
}

void Layer2::doStartup()
{
    dll::ILayer2::doStartup();

}

void Layer2::onNodeCreated()
{
    // Initialize management and control services
    getMSR()->onMSRCreated();
    getCSR()->onCSRCreated();

    fun->onFUNCreated();

    // Create node-wide context providers and set them to zero
    this->setContext("MAC.STAAssociatedToAP", 0);
    this->setContext("MAC.WindowProbeHopCount", 0);

    // Add compound-based context providers
    // TODO: Readin names from configuration
    getNode()->getContextProviderCollection().addProvider(
        wifimac::helper::contextprovider::HopCount(fun, "ForwardingCommand"));
    getNode()->getContextProviderCollection().addProvider(
        wifimac::helper::contextprovider::SourceAddress(fun, "upperConvergence"));
    getNode()->getContextProviderCollection().addProvider(
        wifimac::helper::contextprovider::TargetAddress(fun, "upperConvergence"));
    getNode()->getContextProviderCollection().addProvider(
        wifimac::helper::contextprovider::IsUnicast(fun, "upperConvergence"));
    getNode()->getContextProviderCollection().addProvider(
        wifimac::helper::contextprovider::ModulationCodingScheme(fun, "ManagerCommand"));
    getNode()->getContextProviderCollection().addProvider(
        wifimac::helper::contextprovider::SpatialStreams(fun, "ManagerCommand"));
    getNode()->getContextProviderCollection().addProvider(
        wifimac::helper::contextprovider::IsForMe(fun, "upperConvergence"));

    // Add compound-size context providers
    getNode()->getContextProviderCollection().addProvider(
        wifimac::helper::contextprovider::CompleteLengthInBits());
    getNode()->getContextProviderCollection().addProvider(
        wifimac::helper::contextprovider::CommandPoolLengthInBits());
    getNode()->getContextProviderCollection().addProvider(
        wifimac::helper::contextprovider::DataLengthInBits());

}

void
Layer2::registerManager(wifimac::lowerMAC::Manager* manager,
                        wns::service::dll::UnicastAddress address)
{
    MESSAGE_SINGLE(NORMAL, logger_, "Register manager for transceiver with address " << address);

    managers_.insert(address, manager);

    // Register transceiver as an own station, if it has a different address
    if(address != this->getDLLAddress())
    {
        assure(getStationType() != wns::service::dll::StationTypes::UT(), "UT do not support multiple addresses, because they have no support pathselection");

        this->getStationManager()->registerStation(this->stationID, address, this);

        // Register transceiver at pathselection as an MP (it has no direct
        // connection to RANG)
        std::string psName = config.get<std::string>("pathSelectionServiceName");
        wifimac::pathselection::IPathSelection* ps = this->getManagementService<wifimac::pathselection::IPathSelection>(psName);
        assure(ps, "PathSelection not found");

        ps->registerMP(address);
        // Hop-cost from this transceiver to the DLL is always for free and
        // works in both directions
        ps->createPeerLink(address, this->dll::ILayer2::getDLLAddress(),
                           wifimac::pathselection::Metric::Metric(0));
        ps->createPeerLink(this->dll::ILayer2::getDLLAddress(), address,
                           wifimac::pathselection::Metric(0));
    }
}

bool
Layer2::isTransceiverMAC(wns::service::dll::UnicastAddress address)
{
    // So far only single transceiver
    return (managers_.knows(address));
}

void
Layer2::onWorldCreated()
{
}

void
Layer2::onShutdown()
{
}
