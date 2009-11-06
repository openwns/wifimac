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

    /** @brief Command of wifimac::lowerMAC::Manager */
    class ManagerCommand :
        public wifimac::IKnowsFrameTypeCommand
    {
    public:
        struct {
            /** @brief Time when the packet can be thrown away */
            wns::simulator::Time expirationTime;

            /** @brief Duration after which the reply is missing */
            wns::simulator::Time replyTimeout;
        } local;

        struct {
            wifimac::FrameType type;
            wifimac::convergence::PhyMode phyMode;

            /**
             * @brief Duration field as defined in IEEE 802.11 7.1.3.2
             * i.e. the duration of the frame exchange, without the current
             * frame!
             */
            wns::simulator::Time frameExchangeDuration;
        } peer;

        struct { } magic;

        ManagerCommand()
            {
                peer.type = DATA;
                peer.frameExchangeDuration = 0.0;
                local.replyTimeout = 0.0;
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

        /** @brief Process incoming compounds: Do Nothing at all*/
        void processIncoming(const wns::ldk::CompoundPtr& compound);

        /** @brief Process outgoing compounds: Activate command, set type to
         * DATA and frame exchange duration to SIFS+ACK */
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

        /** @brief Returns the phyUser*/
        wifimac::convergence::PhyUser*
        getPhyUser();

        /** @brief Access the stationType */
        dll::Layer2::StationType
        getStationType() const;

        /** @brief Returns the MAC address is this transceiver. Do not use the
         * similar function from the dll::UpperConvergence - this will give you
         * the MAC address of the complete Layer2 */
        wns::service::dll::UnicastAddress
        getMACAddress() const;

        /** @brief Start association of a STA to the AP with svrAddress */
        void
        associateWith(wns::service::dll::UnicastAddress svrAddress);

        /** @brief Returns the address of the AP to which the STA is associated */
        wns::service::dll::UnicastAddress
        getAssociatedTo() const;

        /** @brief Create-reply function */
        wns::ldk::CommandPool*
        createReply(const wns::ldk::CommandPool* original) const;

        /** @brief Helper function for all FUs in this transceiver to create a
         * new compound with given addresses, type, duration */
        wns::ldk::CompoundPtr
        createCompound(const wns::service::dll::UnicastAddress transmitterAddress,
                       const wns::service::dll::UnicastAddress receiverAddress,
                       const FrameType type,
                       const wns::simulator::Time frameExchangeDuration,
                       const wns::simulator::Time replyTimeout = 0.0);

        /** @brief Getter for the transmitter address of the compound*/
        wns::service::dll::UnicastAddress
        getTransmitterAddress(const wns::ldk::CommandPool* commandPool) const;

        /** @brief Getter for the receiver address of the compound*/
        wns::service::dll::UnicastAddress
        getReceiverAddress(const wns::ldk::CommandPool* commandPool) const;

        /** @brief True if a received frame is intended for me */
        bool
        isForMe(const wns::ldk::CommandPool* commandPool) const;

        /** @brief Get the frame type */
        wifimac::FrameType
        getFrameType(const wns::ldk::CommandPool* commandPool) const;

        /** @brief Set the frame type */
        void
        setFrameType(const wns::ldk::CommandPool* commandPool, const FrameType type);

        /** @brief Get the frame exchange duration
         *
         *  The frame exchange duration denotes the expected duration of the
         *  complete frame exchange, starting from the successful reception of
         *  the compound where this duration is stored. E.g. an RTS contains the
         *  sum of SIFS, CTS, SIFS, DATA, ACK. Nodes overhearing this duration
         *  will set their NAV accordingly.
         *
         *  The frame exchange duration is also used for the duration field in
         *  the preamble
         */
        wns::simulator::Time
        getFrameExchangeDuration(const wns::ldk::CommandPool* commandPool) const;

        /** @brief Set the frame exchange duration */
        void
        setFrameExchangeDuration(const wns::ldk::CommandPool* commandPool, const wns::simulator::Time duration);

        /** @brief Get the PhyMode */
        wifimac::convergence::PhyMode
        getPhyMode(const wns::ldk::CommandPool* commandPool) const;

        /** @brief Set the PhyMode using wifimac::convergence::PhyMode */
        void
        setPhyMode(const wns::ldk::CommandPool* commandPool, const wifimac::convergence::PhyMode phyMode);

        /** @brief Get the "requires direct reply" flag
         *
         *  If a compound requires a direct (i.e. after a short, constant
         *  duration) reply, this bit is set. The receiving node must stop all
         *  other transmission attempts and generate the accorind reply.
         *
         *  An examples is the RTS (requires a CTS after SIFS)
         *
         */
        wns::simulator::Time
        getReplyTimeout(const wns::ldk::CommandPool* commandPool) const;

        /** @brief Set the "requires direct reply" flag */
        void
        setReplyTimeout(const wns::ldk::CommandPool* commandPool, wns::simulator::Time replyTimeout);

        /** @brief Return the number of antennas used for MIMO transmissions */
        unsigned int
        getNumAntennas() const;

        /** @brief Returns if the lifetime limit of the given msdu is expired */
        bool
        lifetimeExpired(const wns::ldk::CommandPool* commandPool) const;

        /** @brief Returns the expiration time of the msdu*/
        wns::simulator::Time
        getExpirationTime(const wns::ldk::CommandPool* commandPool) const;


    private:
        virtual void
        onFUNCreated();

        const wns::pyconfig::View config_;
        wns::logger::Logger logger_;

        /** @brief Expected duration of an ACK frame*/
        const wns::simulator::Time maximumACKDuration;

        /** @brief Short Interframce Space duration */
        const wns::simulator::Time sifsDuration;

        /** @brief my MAC address, as given by the upper convergence */
        const wns::service::dll::UnicastAddress myMACAddress_;

        /** @brief Name of the upper convergence */
        const std::string ucName_;

        /** @brief Number of antennas of the receiver */
        const unsigned int numAntennas;

        /** @brief Lifetime limit of MSDUs*/
        const wns::simulator::Time msduLifetimeLimit;

        /** @brief In case of a STA, the AP to which the STA is associated */
        wns::service::dll::UnicastAddress associatedTo;

        struct Friends
        {
            dll::UpperConvergence* upperConvergence;
            wifimac::convergence::PhyUser* phyUser;
        } friends;
    };
} // lowerMAC
} // wifimac

#endif // WIFIMAC_LOWERMAC_MANAGER_HPP
