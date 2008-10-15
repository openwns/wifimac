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

#include <WIFIMAC/convergence/OFDMAAccessFunc.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>



using namespace wifimac::convergence;

void StopTransmission::operator()()
{
	phyUser->getDataTransmissionService()->stopTransmission( compound, subBand );
}

void StartTransmission::operator()()
{
 	wns::Power defaultTxPower = phyUser->getDataTransmissionService()->getMaxPowerPerSubband();
 	phyUser->getDataTransmissionService()->startUnicast( compound, dstStation, subBand, defaultTxPower );
}

void StartBroadcastTransmission::operator()()
{
 	wns::Power defaultTxPower = phyUser->getDataTransmissionService()->getMaxPowerPerSubband();
	phyUser->getDataTransmissionService()->startBroadcast( compound, subBand, defaultTxPower );
}

void BroadcastOFDMAAccessFunc::operator()(PhyUser* phyUser_, const wns::ldk::CompoundPtr& compound_ )
{
	assure(transmissionStart < transmissionStop, "transmissionStart is larger than transmissionStop");

	StartBroadcastTransmission start ( phyUser_, compound_ );
	wns::simulator::getEventScheduler()->schedule( start, transmissionStart );

	StopTransmission stop ( phyUser_, compound_ );
	wns::simulator::getEventScheduler()->schedule( stop, transmissionStop );
}


void OmniUnicastOFDMAAccessFunc::operator()(PhyUser* phyUser_, const wns::ldk::CompoundPtr& compound_ )
{
	StartTransmission start ( phyUser_, compound_, destination );
	wns::simulator::getEventScheduler()->schedule( start,  transmissionStart );

	StopTransmission stop ( phyUser_, compound_ );
	wns::simulator::getEventScheduler()->schedule( stop, transmissionStop );
}


