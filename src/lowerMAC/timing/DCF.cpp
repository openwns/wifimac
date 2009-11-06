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

#include <WIFIMAC/lowerMAC/timing/DCF.hpp>

#include <WNS/ldk/Layer.hpp>
#include <WNS/ldk/arq/ARQ.hpp>
#include <WNS/service/phy/ofdma/Notification.hpp>
#include <WNS/container/UntypedRegistry.hpp>

#include <DLL/UpperConvergence.hpp>

using namespace wifimac::lowerMAC::timing;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::timing::DCF,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.timing.DCF",
    wns::ldk::FUNConfigCreator);

DCF::DCF(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<DCF, wns::ldk::EmptyCommand>(fun),
    csName(config_.get<std::string>("csName")),
    rxStartEndName(config_.get<std::string>("rxStartEndName")),
    arqCommandName(config_.get<std::string>("arqCommandName")),
    backoffDisabled(config_.get<bool>("myConfig.backoffDisabled")),
    backoff(this, config_),
    sendNow(false),
    logger(config_.get("logger"))
{

} // DCF::DCF


DCF::~DCF()
{
} // DCF::~DCF

void DCF::onFUNCreated()
{
    if(not backoffDisabled)
    {
        // backoff observes the channel state
        backoff.wns::Observer<wifimac::convergence::IChannelState>::startObserving
            (getFUN()->findFriend<wifimac::convergence::ChannelStateNotification*>(csName));

        // backoff gets notified of failed receptions
        backoff.wns::Observer<wifimac::convergence::IRxStartEnd>::startObserving
            (getFUN()->findFriend<wifimac::convergence::RxStartEndNotification*>(rxStartEndName));
    }
} // DCF::onFUNCreated

void
DCF::doSendData(const wns::ldk::CompoundPtr& compound)
{
    assure(sendNow,
           "called doSendData, but sendNow is false");
    sendNow = false;

    assure(getConnector()->hasAcceptor(compound),
           "lower FU is not accepting");

    getConnector()->getAcceptor(compound)->sendData(compound);

}

void
DCF::doOnData(const wns::ldk::CompoundPtr& compound)
{
    // simply forward to the upper FU
    getDeliverer()->getAcceptor(compound)->onData(compound);
}

bool
DCF::doIsAccepting(const wns::ldk::CompoundPtr& compound) const
{
    assure(not backoffDisabled,
           "Backoff was disabled, hence no frames can be accepted");

    if(sendNow and getConnector()->hasAcceptor(compound))
    {
        return true;
    }
    if(getFUN()->getCommandReader(arqCommandName)->commandIsActivated(compound->getCommandPool()))
    {
        int numTransmissions = getFUN()->getCommandReader(arqCommandName)->
            readCommand<wns::ldk::arq::ARQCommand>(compound->getCommandPool())->localTransmissionCounter;
        MESSAGE_SINGLE(NORMAL, logger, "Compound w activated arq command, transmission number " << numTransmissions);
        sendNow = backoff.transmissionRequest(numTransmissions);
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, logger, "Compound w/o activated arq command -> assume first transmission");
        sendNow = backoff.transmissionRequest(1);
    }

    MESSAGE_SINGLE(NORMAL, logger, "backoff asked, sendNow is " << sendNow);

    return(sendNow and getConnector()->hasAcceptor(compound));
}

void
DCF::doWakeup()
{
    getReceptor()->wakeup();
}


void DCF::backoffExpired()
{
    MESSAGE_SINGLE(NORMAL, logger, "Backoff expired, send wakeup");
    this->sendNow = true;
    getReceptor()->wakeup();
}
