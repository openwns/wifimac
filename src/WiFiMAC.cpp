/******************************************************************************
 * WiFiMac (IEEE 802.11 and its alphabet soup)                                *
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

#include <WIFIMAC/WiFiMAC.hpp>
#include <WIFIMAC/bversion.hpp>

#include <WNS/VersionInformation.hpp>

#include <DLL/StationManager.hpp>

#include <WNS/pyconfig/Parser.hpp>
#include <WNS/pyconfig/View.hpp>

using namespace wifimac;

STATIC_FACTORY_REGISTER_WITH_CREATOR(WiFiMAC, wns::module::Base, "wifimac", wns::PyConfigViewCreator);

WiFiMAC::WiFiMAC(const wns::pyconfig::View& config) :
	wns::module::Module<WiFiMAC>(config),
	logger(config.get<wns::pyconfig::View>("logger"))
{
	version = wns::VersionInformation(BUILDVINFO);
}

void WiFiMAC::configure()
{

}

void WiFiMAC::startUp()
{
}

void WiFiMAC::shutDown()
{
}



