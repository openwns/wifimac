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

#ifndef WIFIMAC_LOWERMAC_DUPLICATEFILTER_HPP
#define WIFIMAC_LOWERMAC_DUPLICATEFILTER_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/service/dll/Address.hpp>
#include <WNS/ldk/fu/Plain.hpp>

namespace wifimac { namespace lowerMAC {

	class DuplicateFilterCommand :
        public wns::ldk::Command
	{
    public:
        typedef int64_t SequenceNumber;

        struct {} local;
        struct {
            SequenceNumber sn;
        } peer;
        struct {} magic;
	};

    /**
     * @brief Filters duplicate compounds by a sequence number
     */
	class DuplicateFilter:
		public wns::ldk::fu::Plain<DuplicateFilter, DuplicateFilterCommand>
	{
	public:
		DuplicateFilter(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);
		virtual ~DuplicateFilter();
	private:
		// CompoundHandlerInterface
		void doSendData(const wns::ldk::CompoundPtr& compound);
		void doOnData(const wns::ldk::CompoundPtr& compound);
		void onFUNCreated();
		bool doIsAccepting(const wns::ldk::CompoundPtr& compound) const;
		void doWakeup();

		wns::logger::Logger logger;
        DuplicateFilterCommand::SequenceNumber nextSN;

        typedef wns::container::Registry<wns::service::dll::UnicastAddress, DuplicateFilterCommand::SequenceNumber> AdrSNMap;
        AdrSNMap lastReceivedSN;

        const std::string managerName;
        const std::string arqCommandName;

        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
        } friends;
	};
} // end namespace convergence
} // end namespace wifimac


#endif // ifndef WIFIMAC_LOWERMAC_DUPLICATEFILTER_HPP
