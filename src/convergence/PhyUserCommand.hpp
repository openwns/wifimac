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

#ifndef WIFIMAC_PHYUSER_COMMAND
#define WIFIMAC_PHYUSER_COMMAND

#include <functional>

#include <WIFIMAC/convergence/OFDMAAccessFunc.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>

#include <WNS/ldk/Command.hpp>

namespace wifimac { namespace convergence {
	class PhyUser;

	class CIRProviderCommand
	{
	public:
		virtual wns::Ratio getCIR() const = 0;
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

			std::auto_ptr<OFDMAAccessFunc> pAFunc;

			wns::Ratio postSINRFactor;

		} local;

		struct {} peer;

		struct {} magic;

		PhyUserCommand()
	    {
		}

		// copy operator
		PhyUserCommand(const PhyUserCommand& other) :
			wns::ldk::Command(),
			CIRProviderCommand()
		{
			local.rxPower            = other.local.rxPower;
			local.interference       = other.local.interference;
			local.postSINRFactor     = other.local.postSINRFactor;

			if (other.local.pAFunc.get())
				local.pAFunc.reset(dynamic_cast<wifimac::convergence::OFDMAAccessFunc*>(other.local.pAFunc->clone()));
		}

        wns::Ratio getCIR() const
        {
            return wns::Ratio::from_dB( local.rxPower.get_dBm() - local.interference.get_dBm() + local.postSINRFactor.get_dB() );
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
} // convergence
} // wifimac

#endif // WIFIMAC_PHYUSER_COMMAND

