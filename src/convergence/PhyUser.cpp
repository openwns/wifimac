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

#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/TxDurationSetter.hpp>
#include <WIFIMAC/helper/CholeskyDecomposition.hpp>

#include <WNS/ldk/fun/FUN.hpp>
#include <WNS/pyconfig/View.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/module/Base.hpp>
#include <WNS/ldk/concatenation/Concatenation.hpp>

#include <boost/numeric/ublas/matrix.hpp>

using namespace wifimac::convergence;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::convergence::PhyUser,
    wns::ldk::FunctionalUnit,
    "wifimac.convergence.PhyUser",
    wns::ldk::FUNConfigCreator);

PhyUser::PhyUser(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& _config) :
    wns::ldk::fu::Plain<PhyUser, PhyUserCommand>(fun),
    config(_config),
    logger(config.get<wns::pyconfig::View>("logger")),
    tune(),
    dataTransmission(NULL),
    notificationService(NULL),
    phyModes(config.getView("myConfig.phyModesDeliverer")),
    managerName(config.get<std::string>("managerName")),
    txDurationProviderCommandName(config.get<std::string>("txDurationProviderCommandName")),
    txrxTurnaroundDelay(config.get<wns::simulator::Time>("myConfig.txrxTurnaroundDelay")),
    mimoCorrelation(config.get<double>("myConfig.mimoCorrelation")),
    phyUserStatus(receiving),
    currentTxCompound()
{
    tune.frequency = config.get<double>("myConfig.initFrequency");
    tune.bandwidth = config.get<double>("myConfig.initBandwidthMHz");
    tune.numberOfSubCarrier = 1;
}
// PhyUser


PhyUser::~PhyUser()
{ }

void PhyUser::onFUNCreated()
{
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);

} // onFUNCreated

bool PhyUser::doIsAccepting(const wns::ldk::CompoundPtr& /* compound */) const
{
    // we only accept if we are not currently transmitting
    return (phyUserStatus != transmitting);
} // isAccepting

void PhyUser::doSendData(const wns::ldk::CompoundPtr& compound)
{
    assure(compound, "sendData called with an invalid compound.");
    assure(phyUserStatus != transmitting, "Cannot send data during transmission");

    if(friends.manager->getFrameType(compound->getCommandPool()) != PREAMBLE)
    {
        // signal tx start to the MAC
        this->wns::Subject<ITxStartEnd>::forEachObserver(OnTxStartEnd(compound, start));
    }

    wns::simulator::Time frameTxDuration = getFUN()->getCommandReader(txDurationProviderCommandName)->
        readCommand<wifimac::convergence::TxDurationProviderCommand>(compound->getCommandPool())->getDuration();

    PhyUserCommand* command = activateCommand(compound->getCommandPool());

    // generate functor
    wifimac::convergence::BroadcastOFDMAAccessFunc* func = new wifimac::convergence::BroadcastOFDMAAccessFunc;

    // transmit now
    func->transmissionStart = wns::simulator::getEventScheduler()->getTime();
    func->transmissionStop = wns::simulator::getEventScheduler()->getTime() + frameTxDuration;
    func->subBand = 1;

    command->local.pAFunc.reset(func);
    (*command->local.pAFunc.get())(this, compound);

    MESSAGE_SINGLE(NORMAL, logger, "Transmission, rx disabled for " << frameTxDuration);
    if(phyUserStatus == txrxTurnaround)
    {
        setNewTimeout(frameTxDuration);
    }
    else
    {
        setTimeout(frameTxDuration);
    }
    // we are transmitting!
    phyUserStatus = transmitting;

    this->currentTxCompound = compound;

} // doSendData

void PhyUser::doOnData(const wns::ldk::CompoundPtr& compound)
{
    assure(compound, "onData called with an invalid compound.");

    getDeliverer()->getAcceptor(compound)->onData(compound);
} // doOnData

void PhyUser::doWakeup()
{
    assure(false, "PhyUser doWakeup will never be called -- nobody is below");
} // doWakeup

void PhyUser::onData(wns::osi::PDUPtr pdu, wns::service::phy::power::PowerMeasurementPtr rxPowerMeasurement)
{
    assure(wns::dynamicCast<wns::ldk::Compound>(pdu), "not a CompoundPtr");

    if(phyUserStatus != receiving)
    {
        // During transmission, the receiver is off
        return;
    }

    // FIRST: create a copy instead of working on the real compound
    wns::ldk::CompoundPtr compound = wns::staticCast<wns::ldk::Compound>(pdu)->copy();

    if(!getFUN()->getProxy()->commandIsActivated(
        compound->getCommandPool(), this))     
            return;

    // check if we have enough antennas to receive all streams
    unsigned int nss = friends.manager->getPhyMode(compound->getCommandPool()).getNumberOfSpatialStreams();
    if(nss > friends.manager->getNumAntennas())
    {
        MESSAGE_BEGIN(NORMAL, logger, m, "Transmission ");
        m << "from " << friends.manager->getTransmitterAddress(compound->getCommandPool());
        m << " has " << nss;
        m << " spatial streams, but receiver has only " << friends.manager->getNumAntennas() << " antennas -> drop";
        MESSAGE_END();
        return;
    }
    else
    {
        MESSAGE_BEGIN(NORMAL, logger, m, "Transmission ");
        m << "from " << friends.manager->getTransmitterAddress(compound->getCommandPool());
        m << " has " << nss;
        m << " spatial streams, receiver has " << friends.manager->getNumAntennas() << " antennas";
        m << " -> postSINRFactor = " << this->getExpectedPostSINRFactor(nss, friends.manager->getNumAntennas());
        MESSAGE_END();
    }

    PhyUserCommand* phyCommand = getCommand(compound->getCommandPool());

    // store measured signal into Command
    phyCommand->local.rxPower      = rxPowerMeasurement->getRxPower();
    phyCommand->local.interference = rxPowerMeasurement->getInterferencePower();
    phyCommand->local.postSINRFactor = this->getExpectedPostSINRFactor(nss, friends.manager->getNumAntennas());

    this->wns::ldk::FunctionalUnit::onData(compound);
} // onData

wns::Ratio PhyUser::getExpectedPostSINRFactor(unsigned int nss, unsigned int numRx)
{
    assure(numRx >= nss, "Nss must be smaller or equal to numRx");

    double cF = 1.0;

    if(mimoCorrelation > 0.0)
    {
        // there is a correlation among the MIMO channels, thus we have a
        // correlation factor < 1.0

        // generate covariance matrix
        boost::numeric::ublas::matrix<double> m(nss, nss);
        for(unsigned int i = 0; i < nss; ++i)
        {
            for(unsigned int j = 0; j < nss; ++j)
            {
                if(i == j)
                {
                    m(i,j) = 1;
                }
                else
                {
                    m(i,j) = mimoCorrelation;
                }
            }
        }

        // compute correlation matrix & correlation factor
        choleskyDecompose< boost::numeric::ublas::matrix<double> >(m);
        cF = m(nss-1, nss-1)*m(nss-1, nss-1);
    }

    return( wns::Ratio::from_factor(cF * (static_cast<double>(numRx - nss + 1)) / (static_cast<double>(nss))));
}

void PhyUser::onTimeout()
{
    assure(phyUserStatus != receiving, "Timeout although not transmitting");
    if(phyUserStatus == transmitting)
    {
        // finished transmission, start turnaround
        MESSAGE_SINGLE(NORMAL, logger, "Timout, finished transmission");
        phyUserStatus = txrxTurnaround;
        setTimeout(txrxTurnaroundDelay);

        assure(this->currentTxCompound, "currentTxCompound is NULL");
        if(friends.manager->getFrameType(this->currentTxCompound->getCommandPool()) != PREAMBLE)
        {
            // signal tx end to MAC
            MESSAGE_SINGLE(NORMAL, logger, "No preamble -> signal tx end");
            this->wns::Subject<ITxStartEnd>::forEachObserver(OnTxStartEnd(this->currentTxCompound, end));
        }

        this->currentTxCompound = wns::ldk::CompoundPtr();

        // wakeup: we are ready to transmit another compound
        this->getReceptor()->wakeup();
        return;
    }

    if(phyUserStatus == txrxTurnaround)
    {
        // finished turnaround, ready to receive
        phyUserStatus = receiving;
        return;
    }

    assure(false, "Unknown phyUserStatus");
}

void PhyUser::setDataTransmissionService(wns::service::Service* phy)
{
    assure(phy, "must be non-NULL");
    assureType(phy, wns::service::phy::ofdma::DataTransmission*);
    dataTransmission = dynamic_cast<wns::service::phy::ofdma::DataTransmission*>(phy);
    dataTransmission->setRxTune(tune);
    dataTransmission->setTxTune(tune);
} // setDataTransmissionService

wns::service::phy::ofdma::DataTransmission* PhyUser::getDataTransmissionService() const
{
    assure(dataTransmission, "no ofdma::DataTransmission set. Did you call setDataTransmission()?");
    return dataTransmission;
} // getDataTransmissionService

void PhyUser::setNotificationService(wns::service::Service* phy)
{
    assure(phy, "must be non-NULL");
    assureType(phy, wns::service::phy::ofdma::Notification*);
    notificationService = dynamic_cast<wns::service::phy::ofdma::Notification*>(phy);

    // attach handler (there can be only one)
    notificationService->registerHandler(this);

} // setNotificationService

wns::service::phy::ofdma::Notification* PhyUser::getNotificationService() const
{
    assure(notificationService, "no ofdma::Notification set. Did you call setNotificationService()?");
    return notificationService;
} // getNotificationService

void PhyUser::setFrequency(double frequency)
{
    if(dataTransmission == NULL)
    {
        MESSAGE_SINGLE(NORMAL, logger, "cannot yet set RxFrequency, save for after inialisation");
        tune.frequency = frequency;
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, logger, "setRxFrequency to f: " << frequency << " MHz");
        tune.frequency = frequency;
        dataTransmission->setRxTune(tune);
        dataTransmission->setTxTune(tune);
    }
}

PhyModeProvider* PhyUser::getPhyModeProvider()
{
    return &phyModes;
}


