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

#include <WIFIMAC/management/PERInformationBase.hpp>

#include <WNS/simulator/Time.hpp>
#include <WNS/logger/Logger.hpp>



using namespace wifimac::management;

STATIC_FACTORY_REGISTER_WITH_CREATOR(wifimac::management::PERInformationBase,
                                     wns::ldk::ManagementServiceInterface,
                                     "wifimac.management.PERInformationBase",
                                     wns::ldk::MSRConfigCreator);

PERInformationBase::PERInformationBase( wns::ldk::ManagementServiceRegistry* msr, const wns::pyconfig::View& config):
	wns::ldk::ManagementService(msr),
	logger(config.get("logger")),
    windowSize(config.get<simTimeType>("myConfig.windowSize")),
    minSamples(config.get<int>("myConfig.minSamples")),
    frameSizeThreshold(config.get<Bit>("myConfig.frameSizeThreshold"))
{

}

void
PERInformationBase::onMSRCreated()
{
    MESSAGE_SINGLE(NORMAL, logger, "Created.");
}

void PERInformationBase::reset(const wns::service::dll::UnicastAddress receiver)
{
    assure(receiver.isValid(), "address is not valid");

    if(perHolder.knows(receiver))
    {
        perHolder.find(receiver)->reset();
    }
}

void PERInformationBase::onSuccessfullTransmission(const wns::service::dll::UnicastAddress receiver)
{
    assure(receiver.isValid(), "address is not valid");

    if(!perHolder.knows(receiver))
    {
        perHolder.insert(receiver, new wns::SlidingWindow(windowSize));
    }
    perHolder.find(receiver)->put(0.0);
}


void PERInformationBase::onFailedTransmission(const wns::service::dll::UnicastAddress receiver)
{
    assure(receiver.isValid(), "address is not valid");

    if(!perHolder.knows(receiver))
    {
        perHolder.insert(receiver, new wns::SlidingWindow(windowSize));
    }
    perHolder.find(receiver)->put(1.0);
}

bool PERInformationBase::knowsPER(const wns::service::dll::UnicastAddress receiver) const
{
    assure(receiver.isValid(), "address is not valid");

    if(perHolder.knows(receiver))
    {
        return(perHolder.find(receiver)->getNumSamples() >= minSamples);
    }
    else
    {
        return(false);
    }
}

double PERInformationBase::getPER(const wns::service::dll::UnicastAddress receiver) const
{
    assure(receiver.isValid(), "address is not valid");

    assure(knowsPER(receiver), "Success rate for destination " << receiver << " not known");
    return(perHolder.find(receiver)->getAbsolute() / perHolder.find(receiver)->getNumSamples());
}


