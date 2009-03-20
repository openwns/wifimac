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

#include <WIFIMAC/lowerMAC/NextAggFrameGetter.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <DLL/Layer2.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::NextAggFrameGetter,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.NextAggFrameGetter",
    wns::ldk::FUNConfigCreator);

NextAggFrameGetter::NextAggFrameGetter(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
   wifimac::lowerMAC::NextFrameGetter(fun, config_),
   numOfCompounds(0),
   PSDUSize(0),
   bufferName(config_.get<std::string>("bufferName")),
   protocolCalculatorName(config_.get<std::string>("protocolCalculatorName"))
{
  friends.buffer = NULL;
  protocolCalculator = NULL;
}

void NextAggFrameGetter::onFUNCreated()
{
    protocolCalculator = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);
    friends.buffer = getFUN()->findFriend<wifimac::lowerMAC::MultiBuffer*>(bufferName);
}

void
NextAggFrameGetter::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    std::vector<Bit> compoundSizes;
    int32_t i;

    NextFrameGetter::processOutgoing(compound);

    // first of compound block to be treaded as aggregated
    if (numOfCompounds == 0) 
    {
	// number of compounds of the aggregated frame
	numOfCompounds = 1;
	compoundSizes = friends.buffer->getCurrentBufferSizes();
	numOfCompounds += compoundSizes.size();
	compoundSizes.push_back(compound->getLengthInBits());
        // calculated size of the aggregated frame
	PSDUSize = protocolCalculator->getFrameLength()->getA_MSDU_PSDU(compoundSizes);
    }	
    numOfCompounds--;
} // processOutgoing

Bit 
NextAggFrameGetter::getNextSize() const
{
	return PSDUSize;
}
