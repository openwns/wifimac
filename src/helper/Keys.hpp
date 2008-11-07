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

#ifndef WIFIMAC_HELPER_KEYS_HPP
#define WIFIMAC_HELPER_KEYS_HPP

#include <WNS/ldk/Compound.hpp>
#include <WNS/ldk/fun/FUN.hpp>
#include <WNS/ldk/Key.hpp>

#include <DLL/UpperConvergence.hpp>

namespace wifimac { namespace helper {

	/** @brief derived from wns::ldk::Key to disable flow separation */
	class NoKey :
		public wns::ldk::Key
	{
	public:
		bool operator<(const wns::ldk::Key& /*other*/) const
			{
				return false;
			}
		std::string str() const	// FIXME: should be streaming
			{
				std::stringstream ss;
				ss << "noKey";
				return ss.str();
			}
	};

	/** @brief wns::ldk::KeyBuilder for NoKey */
	class NoKeyBuilder :
		public wns::ldk::KeyBuilder
	{
	public:
		NoKeyBuilder(const wns::ldk::fun::FUN* /*fun*/, const wns::pyconfig::View& /*config*/){}; 
		virtual void onFUNCreated(){};
		virtual wns::ldk::ConstKeyPtr operator () (const wns::ldk::CompoundPtr& /*compound*/, int /*direction*/) const
			{
				return wns::ldk::ConstKeyPtr(new NoKey());
			}
	};

	class LinkByReceiverBuilder;

	class LinkByReceiver:
		public wns::ldk::Key
	{
	public:
		LinkByReceiver(const LinkByReceiverBuilder* factory, const wns::ldk::CompoundPtr& compound, int /*direction*/);
		LinkByReceiver(wns::service::dll::UnicastAddress _linkId);
	    bool operator<(const wns::ldk::Key& _other) const;
		std::string str() const;	// FIXME(fds): should be streaming

		wns::service::dll::UnicastAddress linkId;
	};

	/** @brief wns::ldk::KeyBuilder for the LinkByReceiver key */
	class LinkByReceiverBuilder :
		public wns::ldk::KeyBuilder
	{
	public:
		LinkByReceiverBuilder(const wns::ldk::fun::FUN* _fun, const wns::pyconfig::View& config);
		virtual void onFUNCreated();
		virtual wns::ldk::ConstKeyPtr operator () (const wns::ldk::CompoundPtr& compound, int direction) const;

		const wns::ldk::fun::FUN* fun;
	};

	class LinkByTransmitterBuilder;

	class LinkByTransmitter:
		public wns::ldk::Key
	{
	public:
		LinkByTransmitter(const LinkByTransmitterBuilder* factory, const wns::ldk::CompoundPtr& compound, int /*direction*/);
		LinkByTransmitter(wns::service::dll::UnicastAddress _linkId);
	    bool operator<(const wns::ldk::Key& _other) const;
		std::string str() const;	// FIXME(fds): should be streaming

		wns::service::dll::UnicastAddress linkId;
	};

	/** @brief wns::ldk::KeyBuilder for the LinkByTransmitter key */
	class LinkByTransmitterBuilder :
		public wns::ldk::KeyBuilder
	{
	public:
		LinkByTransmitterBuilder(const wns::ldk::fun::FUN* _fun, const wns::pyconfig::View& config);
		virtual void onFUNCreated();
		virtual wns::ldk::ConstKeyPtr operator () (const wns::ldk::CompoundPtr& compound, int direction) const;

		const wns::ldk::fun::FUN* fun;
	};

	class TransmitterReceiverBuilder;
	/* @brief derived from wns::ldk::Key to achieve Flow separation based on the
	 * MAC Addresses of transmitter and receiver (extracted from unicast/broadcast command)
	 */

	class TransmitterReceiver :
		public wns::ldk::Key
	{
	public:
		TransmitterReceiver(const TransmitterReceiverBuilder* factory, const wns::ldk::CompoundPtr& compound, int /*direction*/);
		TransmitterReceiver(wns::service::dll::UnicastAddress txAdr, wns::service::dll::UnicastAddress rxAdr);
	    bool operator<(const wns::ldk::Key& _other) const;
		std::string str() const;	// FIXME(fds): should be streaming

		wns::service::dll::UnicastAddress tx;
		wns::service::dll::UnicastAddress rx;
	};

	/** @brief wns::ldk::KeyBuilder for the TransmitterReceiver key */
	class TransmitterReceiverBuilder :
		public wns::ldk::KeyBuilder
	{
	public:
		TransmitterReceiverBuilder(const wns::ldk::fun::FUN* _fun, const wns::pyconfig::View& config);
		virtual void onFUNCreated();
		virtual wns::ldk::ConstKeyPtr operator () (const wns::ldk::CompoundPtr& compound, int direction) const;

		const wns::ldk::fun::FUN* fun;
	};

} // Helper
} // WiFiMac

#endif 
