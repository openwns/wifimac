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

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/ARF.hpp>
#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>

#include <algorithm>

using namespace wifimac::lowerMAC::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(ARF, IRateAdaptationStrategy, "ARF", IRateAdaptationStrategyCreator);

ARF::ARF(
    const wns::pyconfig::View& _config,
    wns::service::dll::UnicastAddress _receiver,
    wifimac::management::PERInformationBase* _per,
    wifimac::management::SINRInformationBase* _sinr,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    IRateAdaptationStrategy(_config, _receiver, _per, _sinr, _manager, _phyUser, _logger),
    myReceiver(_receiver),
    per(_per),
    arfTimer(_config.get<wns::simulator::Time>("arfTimer")),
    exponentialBackoff(_config.get<bool>("exponentialBackoff")),
    initialSuccessThreshold(_config.get<int>("initialSuccessThreshold")),
    maxSuccessThreshold(_config.get<int>("maxSuccessThreshold")),
    successThreshold(_config.get<int>("initialSuccessThreshold")),
    probePacket(false),
    logger(_logger),
    timeout(false)
{
    friends.phyUser = _phyUser;
    curPhyMode = _config.getView("initialPhyMode");
}

wifimac::convergence::PhyMode
ARF::getPhyMode(size_t numTransmissions, const wns::Ratio /*lqm*/) const
{
    return(this->getPhyMode(numTransmissions));
}

wifimac::convergence::PhyMode
ARF::getPhyMode(size_t numTransmissions) const
{
    wifimac::convergence::PhyMode pm = curPhyMode;

    if(timeout)
    {
        friends.phyUser->getPhyModeProvider()->mcsUp(pm);
        return pm;
    }

    if((probePacket and numTransmissions == 2) or (numTransmissions >= 3))
    {
        // last one was a probe packet, but did not succeed
        friends.phyUser->getPhyModeProvider()->mcsDown(pm);
        return pm;
    }

    if(per->getSuccessfull(myReceiver) == successThreshold)
    {
        friends.phyUser->getPhyModeProvider()->mcsUp(pm);
        return pm;
    }

    return pm;
}

void
ARF::setCurrentPhyMode(wifimac::convergence::PhyMode pm)
{
    timeout = false;

    wifimac::convergence::PhyMode pmDown = curPhyMode;
    friends.phyUser->getPhyModeProvider()->mcsDown(pmDown);

    wifimac::convergence::PhyMode pmUp = curPhyMode;
    friends.phyUser->getPhyModeProvider()->mcsUp(pmUp);

    if(curPhyMode == pm)
    {
        probePacket = false;
        return;
    }
    else
    {
        assure(pm == pmDown or pm == pmUp,
               "pm " << pm << " is neither " << pmDown << " nor " << pmUp);

        if(not this->hasTimeoutSet())
        {
            this->cancelTimeout();
        }

        if(pm == pmDown)
        {
            this->setTimeout(arfTimer);

            if(probePacket)
            {
                // last one was a probe packet, but did not succeed
                probePacket = false;
                MESSAGE_SINGLE(NORMAL, *logger, "Last probe packet to " << myReceiver << " did not succeed, going down to MCS "<< pm);
                if(exponentialBackoff and successThreshold < maxSuccessThreshold)
                {
                    successThreshold = successThreshold*2;
                    MESSAGE_SINGLE(NORMAL, *logger, "Set successThreshold to " << successThreshold);
                }
            }
            else
            {
                MESSAGE_SINGLE(NORMAL, *logger, "Failed transmissions to " << myReceiver << " , going down to MCS "<< pm);
                if(exponentialBackoff)
                {
                    successThreshold = successThreshold / 2;
                    if(successThreshold < initialSuccessThreshold)
                    {
                        successThreshold = initialSuccessThreshold;
                    }
                    MESSAGE_SINGLE(NORMAL, *logger, "Set successThreshold to " << successThreshold);
                }
            }
        }
        else
        {
            probePacket = true;
            MESSAGE_SINGLE(NORMAL, *logger, per->getSuccessfull(myReceiver) << " successfull transmissions to " << myReceiver << ", sending probe packet with " << pm);
        }

        per->reset(myReceiver);
        curPhyMode = pm;

    }
}

void
ARF::onTimeout()
{
    timeout = true;
    MESSAGE_SINGLE(NORMAL, *logger, "Timeout, set mcs up to " << curPhyMode);
}
