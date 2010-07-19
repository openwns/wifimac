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
    wifimac::management::PERInformationBase* _per,
    wifimac::management::SINRInformationBase* _sinr,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    IRateAdaptationStrategy(_config, _per, _sinr, _manager, _phyUser, _logger),
    per(_per),
    arfTimer(_config.get<wns::simulator::Time>("arfTimer")),
    exponentialBackoff(_config.get<bool>("exponentialBackoff")),
    initialSuccessThreshold(_config.get<int>("initialSuccessThreshold")),
    maxSuccessThreshold(_config.get<int>("maxSuccessThreshold")),
    successThreshold(_config.get<int>("initialSuccessThreshold")),
    probePacket(false),
    logger(_logger)
{
    friends.phyUser = _phyUser;
    curPhyMode = _config.getView("initialPhyMode");
}

wifimac::convergence::PhyMode
ARF::getPhyMode(const wns::service::dll::UnicastAddress receiver, size_t numTransmissions, const wns::Ratio /*lqm*/) const
{
    return(this->getPhyMode(receiver, numTransmissions));
}

wifimac::convergence::PhyMode
ARF::getPhyMode(const wns::service::dll::UnicastAddress receiver, size_t numTransmissions) const
{
    wifimac::convergence::PhyMode pm = curPhyMode;

    if((probePacket and numTransmissions == 2) or (numTransmissions >= 3))
    {
        // last one was a probe packet, but did not succeed
        friends.phyUser->getPhyModeProvider()->mcsDown(pm);
        return pm;
    }

    if(per->getSuccessfull(receiver) == successThreshold)
    {
        friends.phyUser->getPhyModeProvider()->mcsUp(pm);
        return pm;
    }

    return pm;
}

void
ARF::setCurrentPhyMode(const wns::service::dll::UnicastAddress receiver, wifimac::convergence::PhyMode pm)
{
    wifimac::convergence::PhyMode pmDown = curPhyMode;
    friends.phyUser->getPhyModeProvider()->mcsDown(pmDown);

    wifimac::convergence::PhyMode pmUp = curPhyMode;
    friends.phyUser->getPhyModeProvider()->mcsUp(pmUp);

    myReceiver = receiver;

    if(curPhyMode == pm)
    {
        probePacket = false;
        return;
    }
    else
    {
        curPhyMode = pm;
        assure(pm == pmDown or pm == pmUp,
               "pm " << pm << " is neither " << pmDown << " nor " << pmUp);
        per->reset(receiver);

        if(pm == pmDown)
        {
            if(not this->hasTimeoutSet())
            {
                this->setTimeout(arfTimer);
            }

            if(probePacket)
            {
                // last one was a probe packet, but did not succeed
                probePacket = false;
                MESSAGE_SINGLE(NORMAL, *logger, "Last probe packet to " << receiver << " did not succeed, going down to MCS "<< curPhyMode);
                if(exponentialBackoff and successThreshold < maxSuccessThreshold)
                {
                    successThreshold = successThreshold*2;
                    MESSAGE_SINGLE(NORMAL, *logger, "Set successThreshold to " << successThreshold);
                }
            }
            else
            {
                MESSAGE_SINGLE(NORMAL, *logger, "Failed transmissions to " << receiver << " , going down to MCS "<< curPhyMode);
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
            return;
        }

        if(pm == pmUp)
        {
            probePacket = true;

            if(this->hasTimeoutSet())
            {
                this->cancelTimeout();
            }

            MESSAGE_SINGLE(NORMAL, *logger, per->getSuccessfull(receiver) << " successfull transmissions to " << receiver << ", sending probe packet with " << curPhyMode);
            return;
        }
    }
}

void
ARF::onTimeout()
{
    friends.phyUser->getPhyModeProvider()->mcsUp(curPhyMode);
    per->reset(myReceiver);
    probePacket = true;
    MESSAGE_SINGLE(NORMAL, *logger, "Timeout, set mcs up to " << curPhyMode);

}
