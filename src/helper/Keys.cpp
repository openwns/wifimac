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

#include "Keys.hpp"

#include <WNS/ldk/FlowSeparator.hpp>

#include <sstream>

using namespace wifimac::helper;

STATIC_FACTORY_REGISTER_WITH_CREATOR(NoKeyBuilder,
									 wns::ldk::KeyBuilder,
									 "wifimac.noKey",
									 wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(TransmitterReceiverBuilder,
									 wns::ldk::KeyBuilder,
									 "wifimac.TransmitterReceiver",
									 wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(LinkByReceiverBuilder,
									 wns::ldk::KeyBuilder,
									 "wifimac.LinkByReceiver",
									 wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(LinkByTransmitterBuilder,
									 wns::ldk::KeyBuilder,
									 "wifimac.LinkByTransmitter",
									 wns::ldk::FUNConfigCreator);

// LinkByReceiver Implementation
LinkByReceiver::LinkByReceiver(
	const LinkByReceiverBuilder* factory,
	const wns::ldk::CompoundPtr& compound,
	int direction)
{
	dll::UpperCommand* uc = factory->fun->getCommandReader("upperConvergence")->readCommand<dll::UpperCommand>(compound->getCommandPool());

	if(direction == wns::ldk::Direction::OUTGOING())
	{
		linkId = uc->peer.targetMACAddress;
	}
	else
	{
		linkId = uc->peer.sourceMACAddress;
	}
}

LinkByReceiver::LinkByReceiver(wns::service::dll::UnicastAddress _linkId)
{
	assure(_linkId.isValid(), "Invalid key for linkId");
	linkId = _linkId;
}

bool LinkByReceiver::operator<(const wns::ldk::Key& _other) const
{
	assureType(&_other, const LinkByReceiver*);

	const LinkByReceiver* other = static_cast<const LinkByReceiver*>(&_other);
	return linkId < other->linkId;
}

std::string LinkByReceiver::str() const
{
	std::stringstream ss;
	ss << "LinkId " << linkId;
	return ss.str();
}

LinkByReceiverBuilder::LinkByReceiverBuilder(const wns::ldk::fun::FUN* _fun, const wns::pyconfig::View& /*config*/) :
	fun(_fun)
{
}

void LinkByReceiverBuilder::onFUNCreated()
{

}

wns::ldk::ConstKeyPtr LinkByReceiverBuilder::operator () (const wns::ldk::CompoundPtr& compound, int direction) const
{
	return wns::ldk::ConstKeyPtr(new LinkByReceiver(this, compound, direction));
}

// LinkByTransmitter Implementation
LinkByTransmitter::LinkByTransmitter(
	const LinkByTransmitterBuilder* factory,
	const wns::ldk::CompoundPtr& compound,
	int direction)
{
	dll::UpperCommand* uc = factory->fun->getCommandReader("upperConvergence")->readCommand<dll::UpperCommand>(compound->getCommandPool());

	if(direction == wns::ldk::Direction::OUTGOING())
	{
		linkId = uc->peer.sourceMACAddress;
	}
	else
	{
		linkId = uc->peer.targetMACAddress;
	}
}

LinkByTransmitter::LinkByTransmitter(wns::service::dll::UnicastAddress _linkId)
{
	assure(_linkId.isValid(), "Invalid key for linkId");
	linkId = _linkId;
}

bool LinkByTransmitter::operator<(const wns::ldk::Key& _other) const
{
	assureType(&_other, const LinkByTransmitter*);

	const LinkByTransmitter* other = static_cast<const LinkByTransmitter*>(&_other);
	return linkId < other->linkId;
}

std::string LinkByTransmitter::str() const
{
	std::stringstream ss;
	ss << "LinkId " << linkId;
	return ss.str();
}

LinkByTransmitterBuilder::LinkByTransmitterBuilder(const wns::ldk::fun::FUN* _fun, const wns::pyconfig::View& /*config*/) :
	fun(_fun)
{
}

void LinkByTransmitterBuilder::onFUNCreated()
{

}

wns::ldk::ConstKeyPtr LinkByTransmitterBuilder::operator () (const wns::ldk::CompoundPtr& compound, int direction) const
{
	return wns::ldk::ConstKeyPtr(new LinkByTransmitter(this, compound, direction));
}

// TransmitterReceiver Implementation
TransmitterReceiver::TransmitterReceiver(
	const TransmitterReceiverBuilder* factory,
	const wns::ldk::CompoundPtr& compound,
	int direction)
{
	dll::UpperCommand* uc = factory->fun->getCommandReader("upperConvergence")->readCommand<dll::UpperCommand>(compound->getCommandPool());

	if(direction == wns::ldk::Direction::OUTGOING())
	{
		tx = uc->peer.sourceMACAddress;
		rx = uc->peer.targetMACAddress;
	}
	else
	{
		// reverse the keys, so that replies go to the correct initiator
		rx = uc->peer.sourceMACAddress;
		tx = uc->peer.targetMACAddress;
	}
}

TransmitterReceiver::TransmitterReceiver(wns::service::dll::UnicastAddress txAdr, wns::service::dll::UnicastAddress rxAdr)
{
	assure(txAdr.isValid(), "Invalid key for tx");
	assure(rxAdr.isValid(), "Invalid key for rx");
	tx = txAdr;
	rx = rxAdr;
}

bool TransmitterReceiver::operator<(const wns::ldk::Key& _other) const
{
	assureType(&_other, const TransmitterReceiver*);

	const TransmitterReceiver* other = static_cast<const TransmitterReceiver*>(&_other);
	if (tx == other->tx)
		return rx < other->rx;
	else
		return tx < other->tx;
}

std::string TransmitterReceiver::str() const
{
	std::stringstream ss;
	ss << "TX " << tx << " / RX " << rx;
	return ss.str();
}

TransmitterReceiverBuilder::TransmitterReceiverBuilder(const wns::ldk::fun::FUN* _fun, const wns::pyconfig::View& /*config*/) :
	fun(_fun)
{
}

void TransmitterReceiverBuilder::onFUNCreated()
{

}

wns::ldk::ConstKeyPtr TransmitterReceiverBuilder::operator () (const wns::ldk::CompoundPtr& compound, int direction) const
{
	return wns::ldk::ConstKeyPtr(new TransmitterReceiver(this, compound, direction));
}
