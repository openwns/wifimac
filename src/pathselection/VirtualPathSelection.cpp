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

#include <WIFIMAC/pathselection/VirtualPathSelection.hpp>

#include <DLL/RANG.hpp>

#include <WNS/module/Base.hpp>
#include <WNS/Exception.hpp>
#include <WNS/pyconfig/Sequence.hpp>
#include <WNS/Ttos.hpp>

using namespace wifimac::pathselection;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	wifimac::pathselection::VirtualPathSelection,
	wns::node::component::Interface,
	"wifimac.pathselection.VirtualPathSelection",
	wns::node::component::ConfigCreator);


VirtualPathSelectionService::VirtualPathSelectionService():
	logger("WIFIMAC", "VPS", wns::simulator::getMasterLogger()),
	vps(NULL)
{
	MESSAGE_SINGLE(NORMAL, logger, "Starting VPS Service.");
}

void
VirtualPathSelectionService::setVPS(VirtualPathSelection* _vps)
{
	assure(vps == NULL, "setVPS already called, cannot call twice");
	vps = _vps;
}

VirtualPathSelection*
VirtualPathSelectionService::getVPS()
{
	assureNotNull(vps);
	return(vps);
}

VirtualPathSelection::VirtualPathSelection(wns::node::Interface* _node,	const wns::pyconfig::View& _config) :
	wns::node::component::Component(_node, _config),
	logger(_config.get("logger")),
	numNodes(_config.get<int>("numNodes")),
	pathMatrixIsConsistent(true)
{
	TheVPSService::Instance().setVPS(this);
	MESSAGE_SINGLE(NORMAL, logger, "Starting VPS.");

	// Initialize path and cost matrices
	assure(numNodes > 0, "numNodes is below or equal to zero");
	const addressMatrix::SizeType sizesAM[2] = {numNodes, numNodes};
	paths = addressMatrix(sizesAM, 0);

	const metricMatrix::SizeType sizesMM[2] = {numNodes, numNodes};
	linkCosts = metricMatrix(sizesMM, Metric());
	pathCosts = metricMatrix(sizesMM, Metric());

    if(_config.knows("preKnowledge"))
    {
        preKnowledgeAlpha = _config.get<double>("preKnowledge.alpha");
        assure((preKnowledgeAlpha >= 0.0) and (preKnowledgeAlpha <= 1.0), "preKnowledgeAlpha must be between 0.0 and 1.0");
        MESSAGE_SINGLE(NORMAL, logger, "Existing preKnowledge with alpha " << preKnowledgeAlpha << ", " << _config.get<int>("preKnowledge.size()") << " entries:");

        preKnowledgeCosts = metricMatrix(sizesMM, Metric());

        for (int i=0; i < _config.get<int>("preKnowledge.size()"); ++i)
        {
            std::string s = "preKnowledge.get(" + wns::Ttos(i) + ")";
            wns::pyconfig::Sequence entry = _config.getSequence(s);
            assure(entry.size() == 3, "Entry has wrong size");

            wns::service::dll::UnicastAddress tx = entry.at<wns::service::dll::UnicastAddress>(0);
            int32_t txId = mapper.map(tx);
            wns::service::dll::UnicastAddress rx = entry.at<wns::service::dll::UnicastAddress>(1);
            int32_t rxId = mapper.map(rx);
            double cost = entry.at<double>(2);

            preKnowledgeCosts[txId][rxId] = Metric(cost);
            MESSAGE_SINGLE(NORMAL, logger, i << ":  Added preKnowledge " << tx << "->" << rx << " with cost " << preKnowledgeCosts[txId][rxId]);
        }
    }
}


void
VirtualPathSelection::registerMP(const wns::service::dll::UnicastAddress mpAddress)
{
	assure(!isPortal(mpAddress), "Node with address " << mpAddress << " is already registered as portal and thus automatically as MP");
	assure(!isMeshPoint(mpAddress), "MP with address " << mpAddress << " already registered");
	assure(mapper.getMaxId() < numNodes, "numNodes (" << numNodes << ") is too small for another MP");

	int32_t id = mapper.map(mpAddress);
	// store mp address
	mps.push_back(id);

	// set path to itself
	linkCosts[id][id] = 0;

	MESSAGE_SINGLE(NORMAL, logger, "Added MP " << mpAddress << " to list of MPs, now " << mps.size());
}

void
VirtualPathSelection::registerPortal(const wns::service::dll::UnicastAddress portalAddress, dll::APUpperConvergence* apUC)
{
	assure(!isMeshPoint(portalAddress), "Node with address " << portalAddress << " is already registered as MP, cannot register also as portal");
	assure(!isPortal(portalAddress), "Portal with address " << portalAddress << " already registered");
	assure(mapper.getMaxId() < numNodes, "numNodes (" << numNodes << ") is too small for another Portal");

	int32_t id = mapper.map(portalAddress);

	// first: every portal is also a mp
	mps.push_back(id);

	// set link costs from/to any other portal to zero
	for(adr2ucMap::const_iterator itr = portals.begin(); itr != portals.end(); ++itr)
	{
		linkCosts[itr->first][id] = 0;
		linkCosts[id][itr->first] = 0;
	}

    // set path to itself
	linkCosts[id][id] = 0;

	// store portal address
	portals[id] = apUC;

	MESSAGE_SINGLE(NORMAL, logger, "Added Portal " << portalAddress << " to list of Portals, now " << portals.size());
}

wns::service::dll::UnicastAddress
VirtualPathSelection::getNextHop(const wns::service::dll::UnicastAddress current,
								 const wns::service::dll::UnicastAddress finalDestination)
{
	MESSAGE_SINGLE(NORMAL, logger, "getNextHop query from " << current << " to " << finalDestination);

	if(!pathMatrixIsConsistent)
	{
		this->onNewPathSelectionEntry();
	}

	assure(current.isValid(), "current is not valid");
	assure(finalDestination.isValid(), "finalDestination is not valid");
	assure(isMeshPoint(current), "getNextHop: unknown MP " << current);
	assure(current != finalDestination, "Already reached finalDestination");

	wns::service::dll::UnicastAddress fD;
	if (isMeshPoint(finalDestination))
	{
		fD = finalDestination;
	}
	else
	{
		assure(isMeshPoint(getProxyFor(finalDestination)), finalDestination << " is neither known MP nor proxied by a known MP");
		fD = getProxyFor(finalDestination);
		MESSAGE_SINGLE(NORMAL, logger, "getNextHop: finalDestination " << finalDestination << " is proxied by " << fD);
	}

	if(pathCosts[mapper.get(current)][mapper.get(fD)].isInf())
	{
		MESSAGE_SINGLE(NORMAL, logger, "getNextHop query from " << current << " to " << fD << " is unknown");
		this->printPathSelectionTable();
		return wns::service::dll::UnicastAddress();
	}
	else
	{
		MESSAGE_BEGIN(NORMAL, logger, m, "getNextHop query from ");
		m << current << " to "<< fD;
		m << " --> " << mapper.get(paths[mapper.get(current)][mapper.get(fD)]) << " with total pathcost " << pathCosts[mapper.get(current)][mapper.get(fD)];
		MESSAGE_END();

		return(mapper.get(paths[mapper.get(current)][mapper.get(fD)]));
	}
}

bool
VirtualPathSelection::isMeshPoint(const wns::service::dll::UnicastAddress address) const
{
	assure(address.isValid(), "address is not valid");

	int32_t id;
	if(!mapper.knows(address))
	{
		return false;
	}
	id = mapper.get(address);

	addressList::const_iterator itr;
	for (itr = mps.begin(); itr != mps.end(); ++itr)
	{
		if(*itr == id)
		{
			return true;
		}
	}
	return false;
}

bool
VirtualPathSelection::isPortal(const wns::service::dll::UnicastAddress address) const
{
	assure(address.isValid(), "address is not valid");

	int32_t id;
	if(!mapper.knows(address))
	{
		return false;
	}
	id = mapper.get(address);

	adr2ucMap::const_iterator itr;
	for (itr = portals.begin(); itr != portals.end(); ++itr)
	{
		if(itr->first == id)
		{
			return true;
		}
	}
	return false;
}

wns::service::dll::UnicastAddress
VirtualPathSelection::getPortalFor(const wns::service::dll::UnicastAddress clientAddress)
{
	assure(clientAddress.isValid(), "clientAddress is not valid");

	MESSAGE_SINGLE(NORMAL, logger, "getPortalFor: " << clientAddress);

	if(!pathMatrixIsConsistent)
	{
		this->onNewPathSelectionEntry();
	}


	int32_t id;
	if(!mapper.knows(clientAddress))
	{
		// unknown id
		return wns::service::dll::UnicastAddress();
	}
	id = mapper.get(clientAddress);

	addressMap::const_iterator itr = clients2portals.find(id);
	if(itr == clients2portals.end())
	{
		// not found in list
		return wns::service::dll::UnicastAddress();
	}

	assure(isMeshPoint(mapper.get(itr->second)), mapper.get(itr->second) << " is not a known MP");
	assure(isPortal(mapper.get(itr->second)), mapper.get(itr->second) << " is not a known Portal");

	MESSAGE_SINGLE(NORMAL, logger, "getPortalFor: portal for " << clientAddress << " is known to be " << mapper.get(itr->second));

	return(mapper.get(itr->second));
}

wns::service::dll::UnicastAddress
VirtualPathSelection::getProxyFor(const wns::service::dll::UnicastAddress clientAddress)
{
	assure(clientAddress.isValid(), "clientAddress is not valid");
	MESSAGE_SINGLE(NORMAL, logger, "getProxyFor: proxy for " << clientAddress);

	if(!pathMatrixIsConsistent)
	{
		this->onNewPathSelectionEntry();
	}

	int32_t id;
	if(!mapper.knows(clientAddress))
	{
		// unknown id
		return wns::service::dll::UnicastAddress();
	}
	id = mapper.get(clientAddress);

	addressMap::const_iterator itr = clients2proxies.find(id);
	if(itr == clients2proxies.end())
	{
		// not found in list
		return wns::service::dll::UnicastAddress();
	}

	assure(isMeshPoint(mapper.get(itr->second)), mapper.get(itr->second) << " is not a known MP");

	MESSAGE_SINGLE(NORMAL, logger, "getProxyFor: proxy for " << clientAddress << " is known to be " << mapper.get(itr->second));

	return(mapper.get(itr->second));
}


void
VirtualPathSelection::registerProxy(const wns::service::dll::UnicastAddress proxy,
									const wns::service::dll::UnicastAddress client)
{
	if(!pathMatrixIsConsistent)
	{
		this->onNewPathSelectionEntry();
	}

	assure(client.isValid(), "client address is invalid");
	assure(proxy.isValid(), "proxy address is invalid");
	assure(mapper.knows(proxy), "proxy with address " << proxy << " is not known");
	assure(getProxyFor(client) == wns::service::dll::UnicastAddress(),
		   "client " << client << " is already proxied by " << getProxyFor(client) << ", second proxy by " << proxy << " is not allowed");

	int32_t clientId = mapper.map(client);
	int32_t proxyId = mapper.get(proxy);

	clients2proxies[clientId] = proxyId;
	MESSAGE_SINGLE(NORMAL, logger, "registered " << proxy << " as proxy for " << client);

	if(isPortal(proxy))
	{
		// The best portal is always the proxy itself
		// tell it to the RANG
		portals.find(proxyId)->second->getRANG()->updateAPLookUp(client, portals.find(proxyId)->second);
		clients2portals[clientId] = proxyId;
		MESSAGE_SINGLE(NORMAL, logger, "set portal for " << client << " to " << proxy);
		return;
	}

	// search the best portal for the new client
	// Attention: As the cost for portal->portal is zero, all portals have the same (minimum) cost
	// Hence, we search for the first portal from which the first hop to the proxy is not another portal
	for(adr2ucMap::iterator itr = portals.begin(); itr != portals.end(); ++itr)
	{
		if(!isPortal(getNextHop(mapper.get(itr->first), proxy)))
		{
			assure(pathCosts[proxyId][itr->first].isNotInf(), "Path cost from proxy to portal is inf -> network is not connected");
			assure(pathCosts[itr->first][proxyId].isNotInf(), "Path cost from portal to proxy is inf -> network is not connected");

			// tell it to the RANG
			portals.find(itr->first)->second->getRANG()->updateAPLookUp(client, portals.find(itr->first)->second);
			clients2portals[clientId] = itr->first;
			MESSAGE_SINGLE(NORMAL, logger, "set portal for " << client << " to " << mapper.get(itr->first));
			return;
		}
	}
	MESSAGE_SINGLE(NORMAL, logger, "no portal for client " << client << " available at this moment");
}

void
VirtualPathSelection::deRegisterProxy(const wns::service::dll::UnicastAddress
#if !defined(WNS_NO_LOGGING) || !defined(NDEBUG)
                                      proxy
#endif
                                      , const wns::service::dll::UnicastAddress client)
{
	assure(mapper.knows(client), "client address is not known");
	assure(mapper.knows(proxy), "proxy address is not known");
	assure(getProxyFor(client) == proxy, "MP " << proxy << " is not a proxy for " << client << " --> cannot deRegisterProxy");

	addressMap::iterator itr = clients2proxies.find(mapper.get(client));
	if(itr == clients2proxies.end())
	{
		// not found in list
		assure(false, "trying to deRegister " << client << " but it has no proxy");
	}

	clients2proxies.erase(itr);
	MESSAGE_SINGLE(NORMAL, logger, "DEregistered " << proxy << " as proxy for " << client);

}

void
VirtualPathSelection::createPeerLink(const wns::service::dll::UnicastAddress myself,
									 const wns::service::dll::UnicastAddress peer,
									 const Metric linkMetric)
{
	assure(mapper.knows(myself), "myself (" << myself << ") is not a known address");
	assure(mapper.knows(peer), "peer (" << peer << ") is not a known address");
	assure(isMeshPoint(myself) , "myself (" << myself << ") is not a known MP");
	assure(isMeshPoint(peer), "peer (" << peer << ") is not a known MP");
	assure(linkMetric.isNotInf(), "createPeerLink with metric == inf");

	if(isPortal(myself) && isPortal(peer))
	{
		MESSAGE_SINGLE(NORMAL, logger, "createPeerLink: Ignore wireless link between " << myself << " and " << peer << ", both are portals");
	}
	else
	{
        int32_t myselfId = mapper.get(myself);
        int32_t peerId = mapper.get(peer);

		assure(linkCosts[myselfId][peerId].isInf(), "createPeerLink with already known linkCosts");

        Metric newLinkMetric = linkMetric;
        if(preKnowledgeCosts[myselfId][peerId].isNotInf())
        {
            newLinkMetric = newLinkMetric * (1.0-preKnowledgeAlpha) + preKnowledgeCosts[myselfId][peerId] * preKnowledgeAlpha;
            MESSAGE_SINGLE(NORMAL, logger, "createPeerLink: " << myself << "->" << peer << " has preKnowledge of " << preKnowledgeCosts[myselfId][peerId] << ", " << linkMetric << "->" << newLinkMetric);
        }

        linkCosts[myselfId][peerId] = newLinkMetric;


        MESSAGE_SINGLE(NORMAL, logger, "createPeerLink: " << myself << " --> " << peer << " costs " << linkCosts[myselfId][peerId]);
		pathMatrixIsConsistent = false;
	}
}


void
VirtualPathSelection::updatePeerLink(const wns::service::dll::UnicastAddress myself,
									 const wns::service::dll::UnicastAddress peer,
									 const Metric linkMetric)
{
	assure(mapper.knows(myself), "myself (" << myself << ") is not a known address");
	assure(mapper.knows(peer), "peer (" << peer << ") is not a known address");
	assure(isMeshPoint(myself) , "myself (" << myself << ") is not a known MP");
	assure(isMeshPoint(peer), "peer (" << peer << ") is not a known MP");

	int32_t myselfId = mapper.get(myself);
	int32_t peerId = mapper.get(peer);

	if(linkCosts[myselfId][peerId].isInf())
	{
		throw wns::Exception("cannot update a link which has costs inf --> must be created first!");
	}

	if(isPortal(myself) && isPortal(peer))
	{
		// ignore wired link
		return;
	}

    Metric newLinkMetric = linkMetric;
    if(preKnowledgeCosts[myselfId][peerId].isNotInf())
    {
        newLinkMetric = linkMetric * (1.0-preKnowledgeAlpha) + preKnowledgeCosts[myselfId][peerId] * preKnowledgeAlpha;
        MESSAGE_SINGLE(NORMAL, logger, "updatePeerLink: " << myself << "->" << peer << " has preKnowledge of " << preKnowledgeCosts[myselfId][peerId] << ", " << linkMetric << "->" << newLinkMetric);
    }

	if(linkCosts[myselfId][peerId] == newLinkMetric)
	{
		// same values, no update required
		return;
	}

	MESSAGE_BEGIN(NORMAL, logger, m, "updatePeerLink: ");
	m << myself << " --> " << peer;
	m << " from costs " << linkCosts[myselfId][peerId];
	m << " to " << newLinkMetric;
	MESSAGE_END();

	linkCosts[myselfId][peerId] = newLinkMetric;

	pathMatrixIsConsistent = false;
}

void
VirtualPathSelection::closePeerLink(const wns::service::dll::UnicastAddress myself,
									const wns::service::dll::UnicastAddress peer)
{
	assure(mapper.knows(myself), "myself (" << myself << ") is not a known address");
	assure(mapper.knows(peer), "peer (" << peer << ") is not a known address");
	assure(isMeshPoint(myself) , "myself (" << myself << ") is not a known MP");
	assure(isMeshPoint(peer), "peer (" << peer << ") is not a known MP");

	int32_t myselfId = mapper.get(myself);
	int32_t peerId = mapper.get(peer);

	if(linkCosts[myselfId][peerId].isInf())
	{
		throw wns::Exception("cannot close a link which has costs inf --> must be created first!");
	}

	if(isPortal(myself) && isPortal(peer))
	{
		// link between two portals cannot be closed
		return;
	}

	linkCosts[myselfId][peerId] = Metric();

	pathMatrixIsConsistent = false;
}

void
VirtualPathSelection::onNewPathSelectionEntry()
{
	const addressMatrix::SizeType sizesMM[2] = {numNodes, numNodes};
	addressMatrix pred = addressMatrix(sizesMM, 0);

	// initialize predecessor and pathCost matrix:
	//   * predecessor: If direct link exists, predecessor is source itself
	//   * pathCost: Same as linkCost if the link is bidirectional, inf
	//     otherwise
	pathCosts = linkCosts;
	for (addressList::const_iterator i = mps.begin(); i != mps.end(); ++i)
	{
		for (addressList::const_iterator j = mps.begin(); j != mps.end(); ++j)
		{
			if((*i != *j) && (linkCosts[*i][*j].isNotInf()))
			{
				pred[*i][*j] = *i;
			}
			if((*i != *j) && (linkCosts[*i][*j].isNotInf()) && (linkCosts[*j][*i].isInf()))
			{
				pathCosts[*i][*j] = Metric();
			}
		}
	}

	// Floyd-Warshall Algorithm to compute all-pairs shortest-path inclusive
	// predecessor matrix in O(nodes^3)
	for (addressList::const_iterator k = mps.begin(); k != mps.end(); ++k)
	{
		for (addressList::const_iterator i = mps.begin(); i != mps.end(); ++i)
		{
			for (addressList::const_iterator j = mps.begin(); j != mps.end(); ++j)
			{
				if(pathCosts[*i][*k].isNotInf() && pathCosts[*k][*j].isNotInf())
				{
					if(pathCosts[*i][*k] + pathCosts[*k][*j] < pathCosts[*i][*j])
					{
						pathCosts[*i][*j] = pathCosts[*i][*k] + pathCosts[*k][*j];
						assure(pathCosts[*i][*j] >= 0, "found negative cycle in graph");

						pred[*i][*j] = pred[*k][*j];
					}
				}
			}
		}
	}

	// convert the predecessor matrix into successor matrix paths
	for (addressList::const_iterator i = mps.begin(); i != mps.end(); ++i)
	{
		for (addressList::const_iterator j = mps.begin(); j != mps.end(); ++j)
		{
			if((*i != *j) && (pathCosts[*i][*j].isNotInf()))
			{
				int32_t firstHopOut = *j;

				while(pred[*i][firstHopOut] != *i)
				{
					firstHopOut = pred[*i][firstHopOut];
				}
				paths[*i][*j] = firstHopOut;
			}
		}
	}

	pathMatrixIsConsistent = true;

	// update portal settings for the clients
	// iterate over all clients
	for (addressMap::const_iterator clientsItr = clients2proxies.begin(); clientsItr != clients2proxies.end(); ++clientsItr)
	{
		// if the proxy is the portal, it remains the portal
		if(isPortal(mapper.get(clientsItr->second)))
		{
			assure(clients2portals[clientsItr->first] == clientsItr->second,
			       "Client " << mapper.get(clientsItr->first) <<
				   " has proxy " << mapper.get(clientsItr->second) << 
				   ", which is a portal but the client is assigned to the portal " << mapper.get(clients2portals[clientsItr->first]));

			MESSAGE_SINGLE(NORMAL, logger, "portal for " << mapper.get(clientsItr->first) << " remains " << mapper.get(clientsItr->second) << " beause it is also its proxy");
			continue;
		}

		// search the best portal for the new client (clientsItr->first)
		// Attention: As the cost for portal->portal is zero, all portals have the same (minimum) cost
		// Hence, we search for the first portal from which the first hop to the proxy is not another portal
		for(adr2ucMap::iterator portalsItr = portals.begin(); portalsItr != portals.end(); ++portalsItr)
		{
			if(!isPortal(getNextHop(mapper.get(portalsItr->first), mapper.get(clientsItr->second))))
			{
				assure(pathCosts[clientsItr->second][portalsItr->first].isNotInf(), "Path cost from proxy to portal is inf -> network is not connected");
				assure(pathCosts[portalsItr->first][clientsItr->second].isNotInf(), "Path cost from portal to proxy is inf -> network is not connected");

				if(portalsItr->first != clients2portals.find(clientsItr->first)->second)
				{
					portalsItr->second->getRANG()->updateAPLookUp(mapper.get(clientsItr->first), portalsItr->second);
					clients2portals[clientsItr->first] = portalsItr->first;
					MESSAGE_SINGLE(NORMAL, logger, "change portal for " << mapper.get(clientsItr->first) << " to " << mapper.get(portalsItr->first));
					break;
				}
				else
				{
					MESSAGE_SINGLE(NORMAL, logger, "portal for " << mapper.get(clientsItr->first) << " remains " << mapper.get(portalsItr->first));
				}
			}
		}
	}

#ifndef WNS_NO_LOGGING
	printPathSelectionTable();
#endif
}

void
VirtualPathSelection::printPathSelectionTable() const
{
	assure(pathMatrixIsConsistent, "Called printPathSelectionTable for inconsistent pathMatrix");

	MESSAGE_SINGLE(NORMAL, logger, "PathSelectionTable:");
	for(addressList::const_iterator itr = mps.begin(); itr != mps.end(); ++itr)
	{
		if(isPortal(mapper.get(*itr)))
		{
			MESSAGE_SINGLE(NORMAL, logger, "AP" << mapper.get(*itr) );
			printPortalInformation(mapper.get(*itr));
		}
		else
		{
			MESSAGE_SINGLE(NORMAL, logger, "MP" << mapper.get(*itr) );
		}
		printProxyInformation(mapper.get(*itr));
		printPathSelectionLine(mapper.get(*itr));
	}
}

void VirtualPathSelection::printPathSelectionLine(const wns::service::dll::UnicastAddress source) const
{
	MESSAGE_BEGIN(NORMAL, logger, m, "routing: ");
	for(addressList::const_iterator itr = mps.begin(); itr != mps.end(); ++itr)
	{
		if(pathCosts[mapper.get(source)][*itr].isInf())
		{
			m << mapper.get(*itr) << " (Inf/-1)\t";
		}
		else
		{
			if(*itr == mapper.get(source))
			{
				m << mapper.get(*itr) << " (0/-1)\t";
			}
			else
			{
				m << mapper.get(*itr) << " (" << pathCosts[mapper.get(source)][*itr] << "/" << mapper.get(paths[mapper.get(source)][*itr]) << ")\t";
			}
		}
	}
	MESSAGE_END();
}

void VirtualPathSelection::printProxyInformation(const wns::service::dll::UnicastAddress proxy) const
{
	MESSAGE_BEGIN(NORMAL, logger, m, "proxied clients: ");
	for(addressMap::const_iterator itr = clients2proxies.begin(); itr != clients2proxies.end(); ++itr)
	{
		if(itr->second == mapper.get(proxy))
		{
			m << mapper.get(itr->first) << " ";
		}
	}
	MESSAGE_END();
}

void VirtualPathSelection::printPortalInformation(const wns::service::dll::UnicastAddress portal) const
{
	MESSAGE_BEGIN(NORMAL, logger, m, "portal for: ");
	for(addressMap::const_iterator itr = clients2portals.begin(); itr != clients2portals.end(); ++itr)
	{
		if(itr->second == mapper.get(portal))
		{
			m << mapper.get(itr->first) << " ";
		}
	}
	MESSAGE_END();
}

VirtualPathSelection::AddressStorage::AddressStorage() :
	nextId(1),
	adr2id(),
	id2adr()
{

}

int32_t
VirtualPathSelection::AddressStorage::map(const wns::service::dll::UnicastAddress adr)
{
    if(!adr2id.knows(adr))
    {
        assure(! id2adr.knows(nextId), "NextId already used?");
        adr2id.insert(adr, nextId);
        id2adr.insert(nextId, adr);
        ++nextId;
        return(nextId-1);
    }

	return(adr2id.find(adr));
}

int32_t
VirtualPathSelection::AddressStorage::get(const wns::service::dll::UnicastAddress adr) const
{
    return(adr2id.find(adr));
}

wns::service::dll::UnicastAddress
VirtualPathSelection::AddressStorage::get(const int32_t id) const
{
    return(id2adr.find(id));
}

bool
VirtualPathSelection::AddressStorage::knows(const wns::service::dll::UnicastAddress adr) const
{
    return(adr2id.knows(adr));
}

bool
VirtualPathSelection::AddressStorage::knows(const int32_t id) const
{
       return(id2adr.knows(id));
}

int32_t
VirtualPathSelection::AddressStorage::getMaxId() const
{
	return(nextId-1);
}
