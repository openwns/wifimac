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
	 * @brief the VirtualPathSelection allows for a path-selection which is based on global information, i.e.
	 *    a theoretical blackboard with r/w access for all nodes. Hence, all control messages of the path-selection
	 *    protocol are performed without any transmission costs and delays. This simulates a perfect path-selection
	 *    which performance depends on the (imperfect) link quality measurement only. Therfore, it can be used to
	 *    - Analyze protocol functions without the side-effects of the path-selection
	 *    - Compare (distributed) path-selection protocols to the optimum
	 */
	class VirtualPathSelection:
		virtual public wns::ldk::Layer,
		public wns::node::component::Component,
		public IPathSelection
	{
		class AddressStorage
		{
			typedef wns::container::Registry<wns::service::dll::UnicastAddress, int32_t> IdLookup;
			typedef wns::container::Registry<int32_t, wns::service::dll::UnicastAddress> AdrLookup;

		public:
			AddressStorage();

            int32_t
            map(const wns::service::dll::UnicastAddress adr);

			int32_t
			get(const wns::service::dll::UnicastAddress adr) const;

			wns::service::dll::UnicastAddress
			get(const int32_t id) const;

            bool
            knows(const wns::service::dll::UnicastAddress adr) const;
            bool
            knows(const int32_t id) const;

            int32_t
            getMaxId() const;

		private:
			int32_t nextId;

			IdLookup adr2id;
			AdrLookup id2adr;
		};

	public:
		VirtualPathSelection(
			wns::node::Interface* _node,
			const wns::pyconfig::View& _config);

		virtual void
		doStartup()
			{};

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

		virtual void
		onShutdown()
			{};

		void
		registerMP(const wns::service::dll::UnicastAddress mpAddress);

		void
		registerPortal(const wns::service::dll::UnicastAddress portalAddress,
					   dll::APUpperConvergence* apUC);

		/**
		 * @brief Implementation of the IPathSelection
		 */
		virtual wns::service::dll::UnicastAddress
		getNextHop(const wns::service::dll::UnicastAddress current,
				   const wns::service::dll::UnicastAddress finalDestination);

		virtual bool
		isMeshPoint(const wns::service::dll::UnicastAddress address) const;

		virtual bool
		isPortal(const wns::service::dll::UnicastAddress address) const;

		virtual wns::service::dll::UnicastAddress
		getPortalFor(const wns::service::dll::UnicastAddress clientAddress);

		virtual wns::service::dll::UnicastAddress
		getProxyFor(const wns::service::dll::UnicastAddress clientAddress);

		virtual void
		registerProxy(const wns::service::dll::UnicastAddress proxy,
					  const wns::service::dll::UnicastAddress client);
		virtual void
		deRegisterProxy(const wns::service::dll::UnicastAddress proxy,
						const wns::service::dll::UnicastAddress client);

		virtual void
		createPeerLink(const wns::service::dll::UnicastAddress myself,
					   const wns::service::dll::UnicastAddress peer,
					   const Metric linkMetric = Metric(1));
		virtual void
		updatePeerLink(const wns::service::dll::UnicastAddress myself,
					   const wns::service::dll::UnicastAddress peer,
					   const Metric linkMetric = Metric(1));
		virtual void
		closePeerLink(const wns::service::dll::UnicastAddress myself,
					  const wns::service::dll::UnicastAddress peer);
	private:
		/**
		 * @brief Output the pathselection table for debug info
		 */
		void printPathSelectionTable() const;
		void printPathSelectionLine(const wns::service::dll::UnicastAddress source) const;
		void printProxyInformation(const wns::service::dll::UnicastAddress proxy) const;
		void printPortalInformation(const wns::service::dll::UnicastAddress portal) const;

		/**
		 * @brief (re)compute the path/cost matrix
		 */
		void onNewPathSelectionEntry();

		/**
		 * @brief the logger
		 */
		wns::logger::Logger logger;

		/**
		 * @brief pathMatrix and costMatrix
		 */
		typedef wns::container::Matrix<int32_t, 2> addressMatrix;
		typedef wns::container::Matrix<Metric, 2> metricMatrix;
		typedef std::map<int32_t, int32_t> addressMap;
		typedef std::map<int32_t, dll::APUpperConvergence*> adr2ucMap;
		typedef std::list<int32_t> addressList;

		addressList mps;
		adr2ucMap portals;
		metricMatrix linkCosts;
		metricMatrix pathCosts;
		addressMatrix paths;
		addressMap clients2proxies;
		addressMap clients2portals;

		AddressStorage mapper;
		int numNodes;
		bool pathMatrixIsConsistent;

        double preKnowledgeAlpha;
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
