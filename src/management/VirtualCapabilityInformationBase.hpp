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

#ifndef WIFIMAC_MANAGEMENT_VIRTUALCAPABILITYINFORMATIONBASE_HPP
#define WIFIMAC_MANAGEMENT_VIRTUALCAPABILITYINFORMATIONBASE_HPP

#include <DLL/UpperConvergence.hpp>

#include <WNS/container/Registry.hpp>
#include <WNS/container/UntypedRegistry.hpp>
#include <WNS/ldk/fun/Main.hpp>
#include <WNS/ldk/Layer.hpp>
#include <WNS/node/component/Component.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/Singleton.hpp>


namespace wifimac { namespace management {

    /**
     * @brief Allows "magic" (simulation-only) information exchange about the
     * node's capabilities
     *
     * The virtual capability information base is a global blackboard accessible
     * by any node in the simulation. It can be used to easily exchange
     * information about capabilities without the (regular) way of exchanging
     * management information requests and replies.
     *
     * One example of the usage of this information base is the exchange of the
     * number of antennas for MIMO transmissions: To determine the optimal
     * number of streams, the transmitter has to know the number of antennas at
     * the receiver. In reality, this information is exchanged via management
     * frames. If for a particular evaluation this information exchange is not
     * relevant for the simulation results itself, it can be made "magic",
     * i.e. use this information base instead transmitting packets.
     */
	class VirtualCapabilityInformationBase:
		virtual public wns::ldk::Layer,
		public wns::node::component::Component
	{
	public:
		VirtualCapabilityInformationBase(
			wns::node::Interface* _node,
			const wns::pyconfig::View& _config);

		virtual void
		doStartup()
			{};

		virtual ~VirtualCapabilityInformationBase()
        {
            delete nodeInformationBase;
            delete defaultValues;
        };

		virtual void
		onNodeCreated()
			{};

		virtual void
		onWorldCreated()
			{};

		virtual void
		onShutdown()
			{};

        bool
        knows(const wns::service::dll::UnicastAddress adr, const std::string& key) const;

        template <typename T>
        T
        get(const wns::service::dll::UnicastAddress adr, const std::string& key) const
        {
            assure(this->knows(adr, key), "Cannot get unknown information of node " << adr << " with key " << key);

            InformationBase* myIB;
            if(nodeInformationBase->knows(adr))
            {
                myIB = nodeInformationBase->find(adr);
                if(not myIB->knows(key))
                {
                    myIB = this->defaultValues;
                }
            }
            else
            {
                myIB = this->defaultValues;
            }

            MESSAGE_SINGLE(NORMAL, logger, "Query for node " << adr << ", key " << key << " --> " << myIB->find<T>(key));

            return(myIB->find<T>(key));
        } // get

        template <typename T>
        void
        set(const wns::service::dll::UnicastAddress adr, const std::string& key, const T value)
        {
            if(not nodeInformationBase->knows(adr))
            {
                // first time information about this node is added
                nodeInformationBase->insert(adr, new InformationBase());
            }

            if(nodeInformationBase->find(adr)->knows(key))
            {
                throw wns::Exception("Cannot set the same information twice");
            }

            nodeInformationBase->find(adr)->insert<T>(key, value);

            MESSAGE_SINGLE(NORMAL, logger, "Node " << adr << " inserted key " << key << " with value " << value);
        } // set

    private:
		/**
		 * @brief the logger
		 */
		wns::logger::Logger logger;

        typedef wns::container::UntypedRegistry<std::string> InformationBase;
        typedef wns::container::Registry<wns::service::dll::UnicastAddress, InformationBase*> NodeBase;

        /**
         * @brief the database
         */
        NodeBase* nodeInformationBase;

        /**
         * @brief the default values database
         */
        InformationBase* defaultValues;
	};

	class VirtualCapabilityInformationBaseService {
	public:
		VirtualCapabilityInformationBaseService();

		VirtualCapabilityInformationBase*
		getVCIB();

		void
		setVCIB(VirtualCapabilityInformationBase* _vcib);

	private:
		wns::logger::Logger logger;
		VirtualCapabilityInformationBase* vcib;
	};

	typedef wns::SingletonHolder<VirtualCapabilityInformationBaseService> TheVCIBService;

} // management
} // wifimac

#endif
