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

#ifndef WIFIMAC_LOWERMAC_MANAGER_HPP
#define WIFIMAC_LOWERMAC_MANAGER_HPP

#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/FrameType.hpp>

#include <WNS/logger/Logger.hpp>

#include <WNS/ldk/Command.hpp>
#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>

#include <WNS/service/dll/Address.hpp>

#include <DLL/Layer2.hpp>
#include <DLL/UpperConvergence.hpp>

namespace wifimac { namespace convergence {
    class PhyUser;
}}

namespace wifimac { namespace lowerMAC {

    class ManagerCommand :
        public wifimac::IKnowsFrameTypeCommand
    {
    public:
        struct { } local;

        struct {
            wifimac::FrameType type;
            wifimac::convergence::PhyMode phyMode;

            // Duration field as defined in IEEE 802.11 7.1.3.2
            // i.e. the duration of the frame exchange, without the current
            // frame!
            wns::simulator::Time frameExchangeDuration;

            /** @brief Frame requires a direct reply from peer */
            bool requiresDirectReply;
        } peer;

        struct { } magic;

        ManagerCommand()
            {
                peer.type = DATA;
                peer.frameExchangeDuration = 0;
                peer.requiresDirectReply = false;
            }

        FrameType getFrameType()
            {
                return peer.type;
            }
        bool isPreamble() const
            {
                return peer.type == PREAMBLE;
            }

        wifimac::convergence::PhyMode getPhyMode() const
        {
            return this->peer.phyMode;
        }
    };

    /**
	 * @brief Management Entity for a single IEEE 802.11 transceiver, i.e. the
	 *        lower MAC
	 *
	 * Here, all management information related to a single IEEE 802.11
	 * transceiver is stored and accessible for
	 *  - FUs inside this transceiver
	 *  - Higher FUs outside of a transceiver implementation
     */
    class Manager :
        public wns::ldk::fu::Plain<Manager, ManagerCommand>,
        public wns::ldk::Processor<Manager>
    {
    public:
        Manager(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        virtual
        ~Manager();

         /** @brief Processor Interface Implementation */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

        wifimac::convergence::PhyUser*
        getPhyUser();

        /** @brief Access the stationType */
        dll::Layer2::StationType
        getStationType() const;

        wns::service::dll::UnicastAddress
        getMACAddress() const;

        void
        associateWith(wns::service::dll::UnicastAddress svrAddress);

        wns::ldk::CommandPool*
        createReply(const wns::ldk::CommandPool* original) const;

        wns::ldk::CompoundPtr
        createCompound(const wns::service::dll::UnicastAddress transmitterAddress,
                       const wns::service::dll::UnicastAddress receiverAddress,
                       const FrameType type,
                       const wns::simulator::Time frameExchangeDuration,
                       const bool requiresDirectReply = false);

        /** @brief Getter for the transmitter address */
        wns::service::dll::UnicastAddress
        getTransmitterAddress(const wns::ldk::CommandPool* commandPool) const;

        /** @brief Getter for the receiver address */
        wns::service::dll::UnicastAddress
        getReceiverAddress(const wns::ldk::CommandPool* commandPool) const;

        /** @brief True if a received frame is intended for me */
        bool
        isForMe(const wns::ldk::CommandPool* commandPool) const;

        /** @brief Setter and getter for the frame type */
        wifimac::FrameType
        getFrameType(const wns::ldk::CommandPool* commandPool) const;
        void
        setFrameType(const wns::ldk::CommandPool* commandPool, const FrameType type);

        /** @brief Setter and getter for the frame exchange duration */
        wns::simulator::Time
        getFrameExchangeDuration(const wns::ldk::CommandPool* commandPool) const;
        void
        setFrameExchangeDuration(const wns::ldk::CommandPool* commandPool, const wns::simulator::Time duration);

        /** @brief Setter and getter for the PhyMode */
        wifimac::convergence::PhyMode
        getPhyMode(const wns::ldk::CommandPool* commandPool) const;
        void
        setPhyMode(const wns::ldk::CommandPool* commandPool, const wifimac::convergence::PhyMode phyMode);
        void
        setPhyMode(const wns::ldk::CommandPool* commandPool, const int phyModeId);

        /** @brief Setter and getter for the directReply */
        bool
        getRequiresDirectReply(const wns::ldk::CommandPool* commandPool) const;
        void
        setRequiresDirectReply(const wns::ldk::CommandPool* commandPool, bool requiresDirectReply);

        /** @brief Return the number of antennas used for MIMO transmissions */
        unsigned int
        getNumAntennas() const;

    private:
        virtual void
        onFUNCreated();

        const wns::pyconfig::View config_;
        wns::logger::Logger logger_;
        const wns::simulator::Time expectedACKDuration;
        const wns::simulator::Time sifsDuration;

        const wns::service::dll::UnicastAddress myMACAddress_;
        const std::string ucName_;

        const unsigned int numAntennas;

        struct Friends
        {
            dll::UpperConvergence* upperConvergence;
            wifimac::convergence::PhyUser* phyUser;
        } friends;
    };
} // lowerMAC
} // wifimac

#endif // WIFIMAC_LOWERMAC_MANAGER_HPP
