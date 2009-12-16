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

#ifndef WIFIMAC_CONVERGENCE_PHYUSER_HPP
#define WIFIMAC_CONVERGENCE_PHYUSER_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/convergence/GuiWriter.hpp>
#include <WIFIMAC/convergence/PhyModeProvider.hpp>
#include <WIFIMAC/convergence/ITxStartEnd.hpp>

#include <WNS/service/phy/ofdma/Handler.hpp>
#include <WNS/service/phy/ofdma/Notification.hpp>
#include <WNS/service/phy/ofdma/DataTransmission.hpp>

#include <WNS/service/dll/Address.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/events/CanTimeout.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Command.hpp>

namespace wifimac { namespace lowerMAC {
        class Manager;
}}

namespace wifimac { namespace convergence {

    class CIRProviderCommand
    {
    public:
        virtual wns::Ratio getCIR(int stream = 0) const = 0;
        virtual wns::Power getRSS() const = 0;
        virtual ~CIRProviderCommand(){}
    };

    class PhyUserCommand :
        public wns::ldk::Command,
        public CIRProviderCommand
    {
    public:
        struct {
            wns::Power rxPower;
            wns::Power interference;
            std::vector<wns::Ratio> postSINRFactor;
        } local;

        struct {} peer;
        struct {} magic;

        wns::Ratio getCIR(int stream = 0) const
        {
            return wns::Ratio::from_dB( local.rxPower.get_dBm() - local.interference.get_dBm() + local.postSINRFactor[stream].get_dB() );
        }

        wns::Ratio getCIRwithoutMIMO() const
        {
            return wns::Ratio::from_dB( local.rxPower.get_dBm() - local.interference.get_dBm());
        }

        wns::Power getRSS() const
            {
                return (local.rxPower);
            }
    };

    /**
	 * @brief Convergence FU to the OFDM(A)-Module
     *
     * The PhyUser represents the lowest FU in the WiFiMAC FUN, it transates the
     * FU-Interface to the notification/request interface of the lower OFDM(A)
     * module.
     *
     * Furthermore, the PhyUser provides some Phy-dependent functions, e.g
     * access to the different PhyModes (MCSs) via the PhyModeProvider.
	 */
    class PhyUser:
        public wns::ldk::fu::Plain<PhyUser, PhyUserCommand>,
        public wns::service::phy::ofdma::Handler,
        public wns::events::CanTimeout,
        public TxStartEndNotification
    {

    public:
        PhyUser(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
        virtual ~PhyUser();

        /** @brief Interface to lower layer wns::service::phy::ofdma::Handler */
        virtual void onData(wns::osi::PDUPtr, wns::service::phy::power::PowerMeasurementPtr);

        /** @brief Handling of the services */
        virtual void setNotificationService(wns::service::Service* phy);
        virtual wns::service::phy::ofdma::Notification* getNotificationService() const;
        virtual void setDataTransmissionService(wns::service::Service* phy);
        virtual wns::service::phy::ofdma::DataTransmission* getDataTransmissionService() const;

        /** @brief Handling of PhyModes */
        PhyModeProvider* getPhyModeProvider();

        /** @brief Frequency tuning */
        void setFrequency(double frequency);

        wns::Ratio getExpectedPostSINRFactor(unsigned int nss, unsigned int numRx);

    private:

        // CompoundHandlerInterface
        virtual void doSendData(const wns::ldk::CompoundPtr& sdu);
        virtual void doOnData(const wns::ldk::CompoundPtr& compound);
        virtual void onFUNCreated();
        virtual bool doIsAccepting(const wns::ldk::CompoundPtr& compound) const;
        virtual void doWakeup();

        // onTimeout realisation
        virtual void onTimeout();

        wns::pyconfig::View config;
        wns::logger::Logger logger;

        wns::service::phy::ofdma::Tune tune;
        wns::service::phy::ofdma::DataTransmission* dataTransmission;
        wns::service::phy::ofdma::Notification* notificationService;
        PhyModeProvider phyModes;

        const std::string managerName;
        const std::string txDurationProviderCommandName;
        const wns::simulator::Time txrxTurnaroundDelay;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;

        enum PhyUserStatus
        {
            transmitting,
            receiving,
            txrxTurnaround
        } phyUserStatus;

        wns::ldk::CompoundPtr currentTxCompound;
        wns::simulator::Time lastTxRxTurnaround;

        GuiWriter* GuiWriter_;
    };

} // namespace convergence
} // namespace wifimac

#endif // NOT defined WIFIMAC_CONVERGENCE_PHYUSER_HPP


