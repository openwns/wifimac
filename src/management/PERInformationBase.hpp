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

#ifndef WIFIMAC_MANAGEMENT_PERINFORMATIONBASE_HPP
#define WIFIMAC_MANAGEMENT_PERINFORMATIONBASE_HPP

#include <WNS/ldk/ManagementServiceInterface.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/SlidingWindow.hpp>
#include <WNS/ldk/Compound.hpp>
#include <WNS/service/dll/Address.hpp>
#include <WNS/ldk/arq/statuscollector/Interface.hpp>

namespace wifimac { namespace management {

    /**
     * @brief Storage of packet error rates
     *
     * The packet error rate (PER) information base can be used to store and
     * access information about the expected PER of a node's link. The
     * statistics are updated for every packet using the two functions
     * - onSuccessfullTransmission()
     * - onFailedTransmission()
     * Access of the (averaged) PER is done by
     * - knowsPER()
     * - getPER()
     * Averaging of the PER is performed by a sliding window with a configurable
     * window duration.
     */
    class PERInformationBase:
        public wns::ldk::ManagementService
    {
    public:
        /** @brief Constructor */
        PERInformationBase( wns::ldk::ManagementServiceRegistry*, const wns::pyconfig::View& config );
        /** @brief Destructor */
        virtual ~PERInformationBase() {};

        /** @brief Reset the current statistics for a given target node */
        void reset(const wns::service::dll::UnicastAddress target);

        /** @brief Signal a successfull transmission to a target node */
        void onSuccessfullTransmission(const wns::service::dll::UnicastAddress target);

        /** @brief Signal a failed transmission to a target node */
        void onFailedTransmission(const wns::service::dll::UnicastAddress target);

        /**
         * @brief Query if the PER to the given target is known
         *
         * To be known, the sliding window for averaging must contain at least
         * minSamples samples
         */
        bool knowsPER(const wns::service::dll::UnicastAddress target) const;

        /** @brief Get the PER to the given target */
        double getPER(const wns::service::dll::UnicastAddress target) const;

        int
        getSuccessfull(const wns::service::dll::UnicastAddress receiver) const;

        int
        getFailed(const wns::service::dll::UnicastAddress receiver) const;

    private:
        /** @brief Initialization */
        void
        onMSRCreated();

        /** @brief Holder type of the sliding windows for the targets */
        typedef wns::container::Registry<wns::service::dll::UnicastAddress, wns::SlidingWindow*> slidingWindowMap;
        typedef wns::container::Registry<wns::service::dll::UnicastAddress, int> intMap;

        /** @brief Holder of the sliding windows, one for each target */
        slidingWindowMap perHolder;

        /** @brief The logger */
        wns::logger::Logger logger;

        /** @brief The duration of the sliding window for averaging */
        const simTimeType windowSize;

        /** @brief The minimum number of samples in the sliding window so that
         * an average can be computed */
        const int minSamples;

        intMap successfull;
        intMap failed;
    };
} // management
} // wifimac

#endif
