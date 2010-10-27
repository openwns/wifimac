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

#ifndef WIFIMAC_PATHSELECTION_MESHFORWARDING_HPP
#define WIFIMAC_PATHSELECTION_MESHFORWARDING_HPP

#include <WIFIMAC/Layer2.hpp>
#include <WIFIMAC/pathselection/ForwardingCommand.hpp>
#include <WIFIMAC/pathselection/IPathSelection.hpp>

#include <WNS/ldk/ManagementServiceInterface.hpp>
#include <WNS/logger/Logger.hpp>

#include <WNS/ldk/Command.hpp>
#include <WNS/ldk/fu/Plain.hpp>

#include <WNS/service/dll/Address.hpp>

#include <DLL/UpperConvergence.hpp>


namespace wifimac { namespace pathselection {

	/**
	 * @brief Forwarding of data frames according to the path-selection table and current associations
	 */
	class MeshForwarding :
		public wns::ldk::fu::Plain<MeshForwarding, ForwardingCommand>
	{
	public:

		MeshForwarding(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

		virtual
		~MeshForwarding();

	private:
		// FunctionalUnit / CompoundHandlerInterface
		virtual bool doIsAccepting(const wns::ldk::CompoundPtr& _compound) const;
		virtual void doSendData(const wns::ldk::CompoundPtr& _compound);
		virtual void doWakeup();
		virtual void doOnData(const wns::ldk::CompoundPtr& _compound);
		virtual void doOnDataAP(const wns::ldk::CompoundPtr& compound, ForwardingCommand*& fc, dll::UpperCommand*& uc);
		virtual bool doOnDataFRS(const wns::ldk::CompoundPtr& compound, ForwardingCommand*& fc, dll::UpperCommand*& uc);

		virtual void onFUNCreated();

		/** @brief Debug-Output */
		void printForwardingInformation(ForwardingCommand* fc, dll::UpperCommand* uc) const;

		const wns::pyconfig::View config;
		wns::logger::Logger logger;

		/** @brief Name of upperConvergence */
		std::string ucName;

        wifimac::Layer2* layer2;
		wifimac::pathselection::IPathSelection* ps;

		/** @brief Maximum number of hops to avoid loops */
		const int dot11MeshTTL;

		/**
		 * @brief Fill in the correct header for frames tx by APs to their STAs, i.e. with
		 *        3-addresses
		 */
		void sendFrameToOwnBSS(const wns::ldk::CompoundPtr& compound,
							   ForwardingCommand*& fc,
							   dll::UpperCommand*& uc,
							   wns::service::dll::UnicastAddress src,
							   wns::service::dll::UnicastAddress dst);

		/**
		 * @brief Send frame over Mesh to a final destination which is MP, uses
		 *        4 addresses. Returns true if destination is known
		 */
		bool sendFrameToMP(const wns::ldk::CompoundPtr& compound,
						   ForwardingCommand*& fc,
						   dll::UpperCommand*& uc,
						   wns::service::dll::UnicastAddress src,
						   wns::service::dll::UnicastAddress mpDst);


		/**
		 * @brief Send frame over Mesh to a final destination which is not a MP, uses
		 *        5 addresses. Returns true if destination is known
		 */
		bool sendFrameToOtherBSS(const wns::ldk::CompoundPtr& compound,
								 ForwardingCommand*& fc,
								 dll::UpperCommand*& uc,
								 wns::service::dll::UnicastAddress src,
								 wns::service::dll::UnicastAddress meshSrc,
								 wns::service::dll::UnicastAddress dst);


		/**
		 *	@brief Partial copy of the compound with all "higher" commands
		 *         activated for forwarding
		 */
		wns::ldk::CompoundPtr
		createForwardingCopy(const wns::ldk::CompoundPtr& compound,
							 ForwardingCommand* originalFC,
							 ForwardingCommand*& copyFC,
							 dll::UpperCommand*& copyUC);
	};
} // mac
} // wifimac

#endif // WIFIMAC_PATHSELECTION_MESHFORWARDING_HPP
