/******************************************************************************
 * WiFiMac                                                                    *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2005-2007                                                    *
 * Lehrstuhl fuer Kommunikationsnetze (ComNets)                               *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: wns@comnets.rwth-aachen.de                                          *
 * www: http://wns.comnets.rwth-aachen.de                                     *
 ******************************************************************************/

#ifndef WIFIMAC_LOWERMAC_RATEADAPTATION
#define WIFIMAC_LOWERMAC_RATEADAPTATION

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/IRateAdaptationStrategy.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/lowerMAC/ITransmissionCounter.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/management/SINRInformationBase.hpp>
#include <WIFIMAC/management/PERInformationBase.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/logger/Logger.hpp>

namespace wifimac { namespace lowerMAC {

    class RateAdaptation:
        public wns::ldk::fu::Plain<RateAdaptation, wns::ldk::EmptyCommand>,
        public wns::ldk::Processor<RateAdaptation>
    {
    public:
        RateAdaptation(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
        ~RateAdaptation();

        wifimac::convergence::PhyMode
        getPhyMode(wns::service::dll::UnicastAddress receiver, size_t numTransmissions);

        wifimac::convergence::PhyMode
        getPhyMode(const wns::ldk::CompoundPtr& compound);

    private:
        void
        onFUNCreated();

        /** @brief Processor Interface */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

        const std::string phyUserName;
        const std::string managerName;
        const std::string arqName;
        const std::string sinrMIBServiceName;
        const std::string perMIBServiceName;

        /// Setting of constant rate for ACK frames
        const bool raForACKFrames;
        const int ackFramesRateId;
        wifimac::convergence::PhyMode ackFramesRate;

        /**
		 * @brief Rate adaption per receiver
         */
        typedef wns::container::Registry<wns::service::dll::UnicastAddress,
                                         wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategy*,
                                         wns::container::registry::DeleteOnErase> rxRateMap;
        rxRateMap rateAdaptation;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
            wifimac::convergence::PhyUser* phyUser;
            wifimac::lowerMAC::ITransmissionCounter* arq;
        } friends;

        wifimac::lowerMAC::rateAdaptationStrategies::RateAdaptationStrategyCreator* raCreator;
        wifimac::management::SINRInformationBase* sinrMIB;
        wifimac::management::PERInformationBase* perMIB;

        wns::logger::Logger logger;

    }; // RateAdaptation
} // lowerMAC
} // wifimac

#endif
