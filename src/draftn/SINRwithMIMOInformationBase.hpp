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

#ifndef WIFIMAC_DRAFTN_SINRWITHMIMOINFORMATIONBASE_HPP
#define WIFIMAC_DRAFTN_SINRWITHMIMOINFORMATIONBASE_HPP

#include <WIFIMAC/management/SINRInformationBase.hpp>


namespace wifimac { namespace draftn {

    /**
	 * @brief Interface of the channel sounding information base service
	 */
    class SINRwithMIMOInformationBase:
        public wifimac::management::SINRInformationBase
    {
    public:
        typedef std::map<unsigned int, std::vector<wns::Ratio> > NumSSToFactorMap;
        typedef wns::container::Registry<wns::service::dll::UnicastAddress, NumSSToFactorMap*> AddressToFactorsMap;

        /** @brief Constructor */
        SINRwithMIMOInformationBase( wns::ldk::ManagementServiceRegistry* msr,
                                     const wns::pyconfig::View& config );

        /** @brief Destructor */
        virtual ~SINRwithMIMOInformationBase()
        {};

        void
        putFactorMeasurement(const wns::service::dll::UnicastAddress tx,
                             const std::vector<wns::Ratio> sinrFactor);

        bool
        knowsMeasuredFactor(const wns::service::dll::UnicastAddress tx,
                            unsigned int numSS) const;

        std::vector<wns::Ratio>
        getMeasuredFactor(const wns::service::dll::UnicastAddress tx,
                          unsigned int numSS) const;

        void
        putPeerFactor(const wns::service::dll::UnicastAddress peer,
                      const std::vector<wns::Ratio> factor);
        bool
        knowsPeerFactor(const wns::service::dll::UnicastAddress peer,
                        unsigned int numSS) const;

        std::vector<wns::Ratio>
        getPeerFactor(const wns::service::dll::UnicastAddress peer,
                      unsigned int numSS) const;

        void
        putFakePeerMIMOFactors(const wns::service::dll::UnicastAddress peer,
                               NumSSToFactorMap allFactors);

        NumSSToFactorMap
        getAllMeasuredFactors(const wns::service::dll::UnicastAddress peer) const;
    private:

        void
        onMSRCreated();

        AddressToFactorsMap factorsMeasurement;
        AddressToFactorsMap peerFactors;

        typedef std::pair<NumSSToFactorMap, wns::simulator::Time> FactorsTimePair;
        typedef wns::container::Registry<wns::service::dll::UnicastAddress, FactorsTimePair*> AddressToFactorsTimeMap;
        AddressToFactorsTimeMap fakePeerFactors;

        /** @brief The logger */
        wns::logger::Logger logger;

    };

} // draftn
} // wifimac

#endif // WIFIMAC_DRAFTN_SINRWITHMIMOINFORMATIONBASE_HPP
