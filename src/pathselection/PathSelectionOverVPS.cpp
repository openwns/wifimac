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

#include <WIFIMAC/pathselection/PathSelectionOverVPS.hpp>
#include <WIFIMAC/Layer2.hpp>
#include <WNS/service/dll/StationTypes.hpp>

using namespace wifimac::pathselection;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	wifimac::pathselection::PathSelectionOverVPS,
	wns::ldk::ManagementServiceInterface,
	"wifimac.pathselection.PathSelectionOverVPS",
	wns::ldk::MSRConfigCreator);

PathSelectionOverVPS::PathSelectionOverVPS( wns::ldk::ManagementServiceRegistry* msr, const wns::pyconfig::View& config):
	wns::ldk::ManagementService(msr),
	logger(config.get("logger")),
	upperConvergenceName(config.get<std::string>("upperConvergenceName"))
{
	MESSAGE_SINGLE(NORMAL, logger, " Created Service");
}

void
PathSelectionOverVPS::onMSRCreated()
{
	assure(getMSR()->getLayer<wifimac::Layer2*>()->getStationType() != wns::service::dll::StationTypes::UT(),
		   "PathSelectionOverVPS cannot belong to a UT");

	assure(TheVPSService::Instance().getVPS(), "No virtual path selection service found");

	this->startObserving(getMSR()->getLayer<wifimac::Layer2*>()->getControlService<dll::services::control::Association>("ASSOCIATION"));

	if(getMSR()->getLayer<wifimac::Layer2*>()->getStationType() == wns::service::dll::StationTypes::AP())
	{
		TheVPSService::Instance().getVPS()->registerPortal(this->getMSR()->getLayer<wifimac::Layer2*>()->getDLLAddress(),
														   this->getMSR()->getLayer<wifimac::Layer2*>()->getFUN()
														   ->findFriend<dll::APUpperConvergence*>(upperConvergenceName));
	}
	else
	{
		TheVPSService::Instance().getVPS()->registerMP(this->getMSR()->getLayer<wifimac::Layer2*>()->getDLLAddress());
	}


	MESSAGE_SINGLE(NORMAL, logger, "Created.");
}

wns::service::dll::UnicastAddress
PathSelectionOverVPS::getNextHop(const wns::service::dll::UnicastAddress current,
								 const wns::service::dll::UnicastAddress finalDestination)
{
	assure(current.isValid(), "Address current is not valid");
	assure(finalDestination.isValid(), "Address finalDestination is not valid");

	return(TheVPSService::Instance().getVPS()->getNextHop(current, finalDestination));
}


bool
PathSelectionOverVPS::isMeshPoint(const wns::service::dll::UnicastAddress address) const
{
	assure(address.isValid(), "Address is not valid");

	return(TheVPSService::Instance().getVPS()->isMeshPoint(address));
}

bool
PathSelectionOverVPS::isPortal(const wns::service::dll::UnicastAddress address) const
{
	assure(address.isValid(), "Address is not valid");

	return(TheVPSService::Instance().getVPS()->isPortal(address));
}

wns::service::dll::UnicastAddress
PathSelectionOverVPS::getPortalFor(const wns::service::dll::UnicastAddress address)
{
	assure(address.isValid(), "Address is not valid");

	return(TheVPSService::Instance().getVPS()->getPortalFor(address));
}

wns::service::dll::UnicastAddress
PathSelectionOverVPS::getProxyFor(const wns::service::dll::UnicastAddress address)
{
	assure(address.isValid(), "Address is not valid");

	return(TheVPSService::Instance().getVPS()->getProxyFor(address));
}

void
PathSelectionOverVPS::registerProxy(const wns::service::dll::UnicastAddress server,
				    const wns::service::dll::UnicastAddress client)
{
	assure(server.isValid(), "Address server is not valid");
	assure(client.isValid(), "Address client is not valid");

	TheVPSService::Instance().getVPS()->registerProxy(server, client);
}

void
PathSelectionOverVPS::registerMP(const wns::service::dll::UnicastAddress address)
{
	assure(address.isValid(), "Address is not valid");

	TheVPSService::Instance().getVPS()->registerMP(address);
}

void
PathSelectionOverVPS::registerPortal(const wns::service::dll::UnicastAddress address,
									 dll::APUpperConvergence* apUC)
{
	assure(address.isValid(), "Address is not valid");

	TheVPSService::Instance().getVPS()->registerPortal(address, apUC);
}

void
PathSelectionOverVPS::deRegisterProxy(const wns::service::dll::UnicastAddress server,
									  const wns::service::dll::UnicastAddress client)
{
	assure(server.isValid(), "Address is not valid");
	assure(client.isValid(), "Address is not valid");

	TheVPSService::Instance().getVPS()->deRegisterProxy(server, client);
}

void
PathSelectionOverVPS::createPeerLink(const wns::service::dll::UnicastAddress myself,
									 const wns::service::dll::UnicastAddress peer,
									 const Metric linkMetric)
{
	assure(myself.isValid(), "Address is not valid");
	assure(peer.isValid(), "Address is not valid");

	TheVPSService::Instance().getVPS()->createPeerLink(myself, peer, linkMetric);
}
void
PathSelectionOverVPS::updatePeerLink(const wns::service::dll::UnicastAddress myself,
									 const wns::service::dll::UnicastAddress peer,
									 const Metric linkMetric)
{
	assure(myself.isValid(), "Address is not valid");
	assure(peer.isValid(), "Address is not valid");

	TheVPSService::Instance().getVPS()->updatePeerLink(myself, peer, linkMetric);
}

void
PathSelectionOverVPS::closePeerLink(const wns::service::dll::UnicastAddress myself,
									const wns::service::dll::UnicastAddress peer)
{
	assure(myself.isValid(), "Address is not valid");
	assure(peer.isValid(), "Address is not valid");

	TheVPSService::Instance().getVPS()->closePeerLink(myself, peer);
}

void PathSelectionOverVPS::onAssociated(const wns::service::dll::UnicastAddress slave,
                                        const wns::service::dll::UnicastAddress master)
{
	assure(slave.isValid(), "Slave address is not valid");
	assure(master.isValid(), "Master address is not valid");

	MESSAGE_SINGLE(NORMAL, logger, slave << " is associated to " << master << ", register as proxy");
	TheVPSService::Instance().getVPS()->registerProxy(master, slave);
}

void PathSelectionOverVPS::onDisassociated(const wns::service::dll::UnicastAddress slave,
                                           const wns::service::dll::UnicastAddress master)
{
	assure(slave.isValid(), "Slave address is not valid");
	assure(master.isValid(), "Master address is not valid");

	TheVPSService::Instance().getVPS()->deRegisterProxy(master, slave);
}


