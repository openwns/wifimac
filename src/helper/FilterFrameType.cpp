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

#include <WIFIMAC/helper/FilterFrameType.hpp>
#include <DLL/compoundSwitch/CompoundSwitch.hpp>
#include <WNS/Exception.hpp>

using namespace wifimac::helper;

STATIC_FACTORY_REGISTER_WITH_CREATOR(FilterFrameType,
									 dll::compoundSwitch::Filter,
									 "wifimac.helper.FilterFrameType",
									 dll::compoundSwitch::CompoundSwitchConfigCreator);

FilterFrameType::FilterFrameType(dll::compoundSwitch::CompoundSwitch* compoundSwitch, wns::pyconfig::View& config) :
    dll::compoundSwitch::Filter(compoundSwitch, config)
{
    this->commandReader = this->compoundSwitch_->getFUN()->getCommandReader(config.get<std::string>("commandName"));

    // set accepting frame type
    std::string frameTypeString = config.get<std::string>("fType");

    // convert to lower case
    std::transform(frameTypeString.begin(), frameTypeString.end(), frameTypeString.begin (), (int(*)(int)) std::tolower);

    // .compare == 0 --> no differences between the strings
    if(frameTypeString.compare("preamble") == 0)
    {
        this->acceptingFrameType = PREAMBLE;
        return;
    }

    if(frameTypeString.compare("data") == 0)
    {
        this->acceptingFrameType = DATA;
        return;
    }

    if(frameTypeString.compare("data_txop") == 0)
    {
        this->acceptingFrameType = DATA_TXOP;
        return;
    }

    if(frameTypeString.compare("ack") == 0)
    {
        this->acceptingFrameType = ACK;
        return;
    }

    if(frameTypeString.compare("beacon") == 0)
    {
        this->acceptingFrameType = BEACON;
        return;
    }

    wns::Exception e;
    e << "Unknown frame type \"" << frameTypeString << " - forgot to implement conversion from string to frame type for new type?";
    throw wns::Exception(e);
}
FilterFrameType::~FilterFrameType()
{
}

void
FilterFrameType::onFUNCreated()
{
}

bool
FilterFrameType::filter(const wns::ldk::CompoundPtr& compound) const
{
    return(this->commandReader->readCommand<wifimac::IKnowsFrameTypeCommand>(compound->getCommandPool())->getFrameType() == this->acceptingFrameType);
}
