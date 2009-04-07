/******************************************************************************
 * Glue                                                                       *
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

#ifndef WIFIMAC_PHYUSER_ACCESSFUNC
#define WIFIMAC_PHYUSER_ACCESSFUNC

#include <WNS/ldk/Compound.hpp>
#include <WNS/service/phy/ofdma/Pattern.hpp>
#include <WNS/Cloneable.hpp>
#include <WNS/logger/Logger.hpp>

namespace wns { namespace node {
	class Interface;
}}

namespace wifimac { namespace convergence {
    class PhyUser;

    /**
     * @brief Encapsulates the access to the OFDM(A)-PHY
     */
    class OFDMAAccessFunc:
        public virtual wns::CloneableInterface
	{
	public:
		virtual void operator()(PhyUser*, const wns::ldk::CompoundPtr& compound) = 0;
		virtual ~OFDMAAccessFunc(){}
		OFDMAAccessFunc():
			transmissionStart(-1.0),
			transmissionStop(-1.0),
			subBand(0)
		{}
		double transmissionStart;
		double transmissionStop;
		int subBand;
	};

	class BroadcastOFDMAAccessFunc:
		public OFDMAAccessFunc,
		public wns::Cloneable<BroadcastOFDMAAccessFunc>
	{
	public:
		virtual void operator()(PhyUser*, const wns::ldk::CompoundPtr&);
	};

	class OmniUnicastOFDMAAccessFunc:
		public OFDMAAccessFunc,
		public wns::Cloneable<OmniUnicastOFDMAAccessFunc>
	{
	public:
		virtual void operator()(PhyUser*, const wns::ldk::CompoundPtr&);
		wns::node::Interface* destination;
	};

	/******************* Events ********************/

	class StopTransmission
	{
	public:
		StopTransmission( PhyUser* phyUser_, const wns::ldk::CompoundPtr& compound_, int subBand_ = 0) :
			phyUser( phyUser_ ),
			compound( compound_ ),
			subBand( subBand_ )
		{}

		void operator()();

	private:
		PhyUser* phyUser;
		wns::ldk::CompoundPtr compound;
		int subBand;
	};

	class StartBroadcastTransmission
	{
	public:
		StartBroadcastTransmission( PhyUser* phyUser_,
									const wns::ldk::CompoundPtr& compound_,
									int subBand_ = 0):
			phyUser( phyUser_ ),
			compound( compound_ ),
			subBand( subBand_ )
		{}

		void operator()();

	protected:
		PhyUser* phyUser;
		wns::ldk::CompoundPtr compound;
		int subBand;
	};

	class StartTransmission
	{
	public:
		StartTransmission( PhyUser* phyUser_, const wns::ldk::CompoundPtr& compound_,
						   wns::node::Interface* dstStation_, int subBand_ = 0
						   ) :
			phyUser( phyUser_ ),
			compound( compound_ ),
			dstStation( dstStation_ ),
			subBand( subBand_ )
		{}

		void operator()();

	protected:
		PhyUser* phyUser;
		wns::ldk::CompoundPtr compound;
		wns::node::Interface* dstStation;
		int subBand;
	};
} // convergence
} // wifimac

#endif
