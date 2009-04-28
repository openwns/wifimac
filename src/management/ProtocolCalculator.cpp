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

#include <WIFIMAC/management/ProtocolCalculator.hpp>

using namespace wifimac::management;

STATIC_FACTORY_REGISTER_WITH_CREATOR(ProtocolCalculator,
                                     wns::ldk::ManagementServiceInterface,
                                     "wifimac.management.ProtocolCalculator",
                                     wns::ldk::MSRConfigCreator);

ProtocolCalculator::ProtocolCalculator( wns::ldk::ManagementServiceRegistry* msr, const wns::pyconfig::View& config_):
    wns::ldk::ManagementService(msr),
    logger(config_.get("logger")),
    config(config_)
{
    errorProbability = new protocolCalculatorPlugins::ErrorProbability();
    frameLength = new protocolCalculatorPlugins::FrameLength(config_.get<wns::pyconfig::View>("myConfig.frameLength"));
    duration = new protocolCalculatorPlugins::Duration(frameLength, config_.get<wns::pyconfig::View>("myConfig.duration"));
}

void
ProtocolCalculator::onMSRCreated()
{
    MESSAGE_SINGLE(NORMAL, logger, "Created.");
}


protocolCalculatorPlugins::ErrorProbability*
ProtocolCalculator::getErrorProbability() const
{
    return errorProbability;
}

protocolCalculatorPlugins::FrameLength*
ProtocolCalculator::getFrameLength() const
{
    return frameLength;
}

protocolCalculatorPlugins::Duration*
ProtocolCalculator::getDuration() const
{
    return duration;
}
