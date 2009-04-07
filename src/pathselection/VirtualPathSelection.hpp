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

#ifndef WIFIMAC_PATHSELECTION_VIRTUALPATHSELECTION_HPP
#define WIFIMAC_PATHSELECTION_VIRTUALPATHSELECTION_HPP

#include <WIFIMAC/pathselection/IPathSelection.hpp>
#include <WIFIMAC/pathselection/Metric.hpp>

#include <DLL/UpperConvergence.hpp>

#include <WNS/node/component/Component.hpp>
#include <WNS/container/Registry.hpp>
#include <WNS/ldk/fun/Main.hpp>
#include <WNS/ldk/Layer.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/Singleton.hpp>
#include <WNS/container/Matrix.hpp>

#include <map>
#include <list>

namespace wifimac { namespace pathselection {

	/**
	 * @brief The Virtual Path Selection implements a perfect path selection
	 * protocol without overhead.
     *
     * The VirtualPathSelection allows for a path-selection which is based on
	 * global information, i.e. a theoretical, simulator-only blackboard with
	 * r/w access for all nodes. Hence, all control messages of the
	 * path-selection protocol are performed without any transmission costs and
	 * delays. This simulates a perfect path-selection whose performance depends
	 * on the (imperfect) link quality measurement only. Therfore, it can be
	 * used to
     * - Analyze protocol functions without the side-effects of the
	 *   path-selection
     * - Compare (distributed) path-selection protocols to the
	 *   optimum
	 */
    class VirtualPathSelection:
        virtual public wns::ldk::Layer,
        public wns::node::component::Component,
        public IPathSelection
    {
        /**
         * @brief AddresStorage provides a mapping from
         * wns::service::dll::UnicastAddress to a (internal) id
         *
         * The mapping to an internal id saves memory space when creating the
         * link cost and path cost matrices, as unicast addresses are mapped to
         * a continuous set of numbers. Also, the matrices' size have to be
         * initialized at startup, and it suffices to get the number of nodes
         * instead the highest node address.
         */
        class AddressStorage
        {
            typedef wns::container::Registry<wns::service::dll::UnicastAddress, int> IdLookup;
            typedef wns::container::Registry<int, wns::service::dll::UnicastAddress> AdrLookup;

        public:
            AddressStorage();

            /**
             * @brief Create new map from adr, returns its id
             */
            int
            map(const wns::service::dll::UnicastAddress adr);

            /**
             * @brief Convert adr to id
             */
            int
            get(const wns::service::dll::UnicastAddress adr) const;

            /**
             * @brief Convert id to adr
             */
            wns::service::dll::UnicastAddress
            get(const int id) const;

            /**
             * @brief Check if adr is known
             */
            bool
            knows(const wns::service::dll::UnicastAddress adr) const;

            /**
             * @brief Check if id is known
             */
            bool
            knows(const int id) const;

            /**
             * @brief Returns the maximum id
             */
            int
            getMaxId() const;

        private:
            /**
             * @brief Counter for the next, unused id
             */
            int nextId;

            /**
             * @brief Map for adr to id
             */
            IdLookup adr2id;
            /**
             * @brief Map for id to adr. Always consistent with adr2id
             */
            AdrLookup id2adr;
        };

    public:
        /**
         * @brief Constructor of virtual node
         */
        VirtualPathSelection(
            wns::node::Interface* _node,
            const wns::pyconfig::View& _config);

        /**
         * @brief Startup routine of virtual node, does nothing
         */
        virtual void
        doStartup()
            {};

        /**
         * @brief Destructor, does nothing
         */
        virtual ~VirtualPathSelection()
            {};

        /**
		 * @brief Find partner components within your node as given by
		 * the configuration.
		 */
        virtual void
        onNodeCreated()
            {};

        /**
         * @brief Find peer components in other nodes.
		 */
        virtual void
        onWorldCreated()
            {};

        /**
         * @brief Called when the simulator is shutdown. Does nothing
         */
        virtual void
        onShutdown()
            {};

        /**
         * @brief Register an address as a Mesh Point
         *
         * The address is stored in the list of MPs and the link cost to itself
         * is set to zero. Calling this member is the initial step for any node
         * in the mesh.
         */
        void
        registerMP(const wns::service::dll::UnicastAddress mpAddress);

        /**
         * @brief Register a Portal
         *
         * The portal is a node which
         * - participates in the mesh, i.e. is able to forward frames
         * - is connected to the Internet (i.e. the Radio Access Network Gateway
         * node)
         * When called, the address is stored in the list of MPs and
         * Portals. Also, the link cost to any other portal is set to zero (as
         * the wired backbone can be used).
         */
        void
        registerPortal(const wns::service::dll::UnicastAddress portalAddress,
                       dll::APUpperConvergence* apUC);

        /**
		 * @brief Returns the next hop from current to finalDestination
         *
         * getNextHop is the most important function in the path selection
         * interface.
         * In a first (optional) step, the finalDestination is exchanged by its
         * mesh proxy, which is a Portal or a MP. Then, the forwarding matrix,
         * as calculated by onNewPathSelectionEntry(), is asked for the next hop
         * to this proxy.
		 */
        virtual wns::service::dll::UnicastAddress
        getNextHop(const wns::service::dll::UnicastAddress current,
                   const wns::service::dll::UnicastAddress finalDestination);

        /**
         * @brief Returns if address is inside the list of known MPs
         */
        virtual bool
        isMeshPoint(const wns::service::dll::UnicastAddress address) const;

        /**
         * @brief Returns if address is inside the list of known Portals
         */
        virtual bool
        isPortal(const wns::service::dll::UnicastAddress address) const;

        /**
         * @brief Returns the Portal for a address
         *
         * Any node in the mesh has exactly one portal, the gateway to the
         * Internet. All data addressed to the Internet is send to this portal.
         */
        virtual wns::service::dll::UnicastAddress
        getPortalFor(const wns::service::dll::UnicastAddress clientAddress);

        /**
         * @brief Returns the proxy of a address
         *
         * Every STA in a mesh network, has a proxy which is responsible for the
         * forwarding of packets to/from this STA. The Proxy acts instead of the
         * STA, which is not able to participate in the mesh; hence, the proxy
         * must be a known MP.
         */
        virtual wns::service::dll::UnicastAddress
        getProxyFor(const wns::service::dll::UnicastAddress clientAddress);

        /**
         * @brief Registers a proxy for a client address.
         *
         * Besides adding the client to the list of clients proxied by the
         * proxy, also the best portal for the client is calculated and told to
         * the RANG: If the proxy is a portal, this is the best one; otherwise,
         * the best portal for the new client is searched by the help of the
         * path selection matrix: The cost for portal->portal is zero. Hence,
         * the one portal is searched for which the cost portal->proxy is > zero.
         */
        virtual void
        registerProxy(const wns::service::dll::UnicastAddress proxy,
                      const wns::service::dll::UnicastAddress client);
        /**
         * @brief Removes the proxy from the clients to proxy mapping
         */
        virtual void
        deRegisterProxy(const wns::service::dll::UnicastAddress proxy,
                        const wns::service::dll::UnicastAddress client);

        /**
         * @brief Initializes a new link with cost linkMetric between the addresses myself and peer.
         */
        virtual void
        createPeerLink(const wns::service::dll::UnicastAddress myself,
                       const wns::service::dll::UnicastAddress peer,
                       const Metric linkMetric = Metric(1));
        /**
         * @brief Updates the cost of an already existing link
         */
        virtual void
        updatePeerLink(const wns::service::dll::UnicastAddress myself,
                       const wns::service::dll::UnicastAddress peer,
                       const Metric linkMetric = Metric(1));

        /**
         * @brief Closes an existing link
         */
        virtual void
        closePeerLink(const wns::service::dll::UnicastAddress myself,
                      const wns::service::dll::UnicastAddress peer);
    private:
        /**
		 * @brief Output the pathselection table for debug info
		 */
        void
        printPathSelectionTable() const;
        /**
         * @brief Helper routine for printPathSelectionTable()
         */
        void
        printPathSelectionLine(const wns::service::dll::UnicastAddress source) const;
        /**
         * @brief Helper routine for printPathSelectionTable()
         */
        void
        printProxyInformation(const wns::service::dll::UnicastAddress proxy) const;
        /**
         * @brief Helper routine for printPathSelectionTable()
         */
        void
        printPortalInformation(const wns::service::dll::UnicastAddress portal) const;

        /**
		 * @brief (re)compute the path/cost matrix
		 */
        void onNewPathSelectionEntry();

        /**
		 * @brief the logger
		 */
        wns::logger::Logger logger;

        /**
		 * @brief 2-dim Matrix of ids
		 */
        typedef wns::container::Matrix<int, 2> addressMatrix;
        /**
		 * @brief 2-dim Matrix of path/link costs
		 */
        typedef wns::container::Matrix<Metric, 2> metricMatrix;
        /**
		 * @brief Map one id to another
		 */
        typedef std::map<int, int> addressMap;
        /**
		 * @brief Map an id to a dll::APUpperConvergence, required by the RANG
		 */
        typedef std::map<int, dll::APUpperConvergence*> adr2ucMap;
        /**
		 * @brief List of addressed
		 */
        typedef std::list<int> addressList;

        /**
		 * @brief Stores all known MPs
		 */
        addressList mps;

        /**
         * @brief Maps the address to the upper convergence entry of a portal.
         */
        adr2ucMap portals;
        /**
         * @brief NxN Matrix of link costs
         */
        metricMatrix linkCosts;
        /**
         * @brief NxN Matrix of path cost
         */
        metricMatrix pathCosts;
        /**
         * @brief Next hop matrix of the optimal path
         */
        addressMatrix paths;
        /**
         * @brief Maps client ids to proxies
         */
        addressMap clients2proxies;
        /**
         * @brief Maps client ids to portals
         */
        addressMap clients2portals;

        /**
         * @brief Mapping of wns::service::dll::UnicastAddress to (internal) ids
         */
        AddressStorage mapper;

        /**
         * @brief Maximum number of nodes, important for the creating of all
         * matrices.
         */
        const int numNodes;

        /**
         * @brief Lazy computation of the path selection matrices
         */
        bool pathMatrixIsConsistent;

        /**
         * @brief Weight parameter of the given link costs
         */
        double preKnowledgeAlpha;
        /**
         * @brief Matrix of given (configured) link costs which allow (partial)
         * overriding of measured link costs
         */
        metricMatrix preKnowledgeCosts;
    };

    class VirtualPathSelectionService {
    public:
        VirtualPathSelectionService();

        VirtualPathSelection*
        getVPS();

        void
        setVPS(VirtualPathSelection* _vps);

    private:
        wns::logger::Logger logger;
        VirtualPathSelection* vps;
    };

    typedef wns::SingletonHolder<VirtualPathSelectionService> TheVPSService;

} // mac
} // wifimac

#endif // WIFIMAC_SCHEDULER_HPP
