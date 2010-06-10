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

#ifndef WIFIMAC_MANAGEMENT_SINRINFORMATIONBASE_HPP
#define WIFIMAC_MANAGEMENT_SINRINFORMATIONBASE_HPP

#include <WNS/ldk/ManagementServiceInterface.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/PowerRatio.hpp>
#include <WNS/SlidingWindow.hpp>

#include <WNS/service/dll/Address.hpp>
#include <WNS/simulator/Time.hpp>

#include <utility>


namespace wifimac { namespace management {

    /**
     * @brief Storage and retrieval of SINR measurements
     *
     * This information base provides a central point in a node to store and
     * access SINR measurements. Two kind of measurements must be
     * differentiated:
     *
     * -# Direct measurements at the node (done e.g. by the PHY layer by
     *    comparing known symbols with received symbols). Methods to store an to
     *    access these measurements are
     *    - putMeasurement()
     *    - knowsMeasuredSINR()
     *    - getAverageMeasuredSINR()
     *    As indicated by the last function, the SINR information base performs
     *    an averaging of the stored SINR values.
     * -# Measurements done at a peer node and transmitted as management
     *    information to this node. Methods for this are
     *    - putPeerSINR()
     *    - knowsPeerSINR()
     *    - getAveragePeerSINR()
     *    Averaging of the peer SINR values is not performed at the node, but at
     *    the peer node.
     */
    class SINRInformationBase:
        public wns::ldk::ManagementService
    {
    public:
        /** @brief Constructor */
        SINRInformationBase( wns::ldk::ManagementServiceRegistry*, const wns::pyconfig::View& config );

        /** @brief Destructor */
        virtual ~SINRInformationBase() {};


        /** @brief Store SINR value measured here (link tx -> me)
         *
         *  The estimatedValidity parameter gives the duration for which this
         *  specific value is accurate. If during this time a getSINR is issued,
         *  exactly this value will be taken; otherwise, the averaged value is
         *  returned.
         */
        void
        putMeasurement(const wns::service::dll::UnicastAddress tx,
                       const wns::Ratio sinr,
                       const wns::simulator::Time estimatedValidity = 0.0);

        /** @brief Ask if SINR value of link tx -> me is known */
        bool
        knowsMeasuredSINR(const wns::service::dll::UnicastAddress tx);

        /** @brief Returns the averaged SINR of the link tx -> me */
        wns::Ratio
        getMeasuredSINR(const wns::service::dll::UnicastAddress tx);


        /**
         * @brief Store received SINR measurement of link me -> peer
         *
         * This link measurement must have been transported to me by regular
         * management data exchange.
         */
        void
        putPeerSINR(const wns::service::dll::UnicastAddress peer,
                    const wns::Ratio sinr,
                    const wns::simulator::Time estimatedValidity = 0.0);

        /** @brief Query if SINR of link me -> peer is known */
        bool
        knowsPeerSINR(const wns::service::dll::UnicastAddress peer);

        /** @brief Get the averaged SINR of link me -> peer */
        wns::Ratio
        getPeerSINR(const wns::service::dll::UnicastAddress peer);

        /** @brief */
        void
        putFakePeerSINR(const wns::service::dll::UnicastAddress peer,
                        const wns::Ratio sinr);

    private:

        /** @brief Initialization */
        void
        onMSRCreated();

        /** @brief Measurement holder type: Maps address to sliding window */
        typedef wns::container::Registry<wns::service::dll::UnicastAddress, wns::SlidingWindow*, wns::container::registry::DeleteOnErase> slidingWindowMap;

        /** @brief Holds SINR values measured here */
        slidingWindowMap measuredSINRHolder;

        /** @brief SINR information holder type: Maps peer address to SINR */
        typedef wns::container::Registry<wns::service::dll::UnicastAddress, wns::Ratio*> ratioMap;

        /** @brief Holds SINR values received by peers */
        ratioMap peerSINRHolder;

        typedef std::pair<wns::Ratio, wns::simulator::Time> ratioTimePair;
        typedef wns::container::Registry<wns::service::dll::UnicastAddress, ratioTimePair*> ratioWithTimeMap;

        /** @brief Holds the last SINR measurement */
        ratioWithTimeMap lastMeasurement;

        /** @brief Holds the last received peer SINR measurement */
        ratioWithTimeMap lastPeerMeasurement;

        /** @brief Holds fake peer measurements */
        ratioWithTimeMap fakePeerMeasurement;

        /** @brief The logger */
        wns::logger::Logger logger;

        /** @brief Duration of the sliding window for averaging the SINR values */
        const simTimeType windowSize;

    };
} // management
} // wifimac

#endif
