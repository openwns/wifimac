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

#include <WIFIMAC/helper/FilterSize.hpp>
#include <DLL/compoundSwitch/CompoundSwitch.hpp>

using namespace wifimac::helper;

STATIC_FACTORY_REGISTER_WITH_CREATOR(FilterSize,
									 dll::compoundSwitch::Filter,
									 "wifimac.helper.FilterSize",
									 dll::compoundSwitch::CompoundSwitchConfigCreator);

FilterSize::FilterSize(dll::compoundSwitch::CompoundSwitch* compoundSwitch, wns::pyconfig::View& config) :
    dll::compoundSwitch::Filter(compoundSwitch, config),
    minSize(config.get<Bit>("minSize")),
    maxSize(config.get<Bit>("maxSize"))
{

}
FilterSize::~FilterSize()
{
}

void
FilterSize::onFUNCreated()
{
}

bool
FilterSize::filter(const wns::ldk::CompoundPtr& compound) const
{
    Bit len = compound->getLengthInBits();
    return((len >= minSize) and (len <= maxSize));
}
