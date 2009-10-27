/******************************************************************************
 * WiFiMAC (IEEE 802.11)                                                      *
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

#ifndef WIFIMAC_HELPER_CONTEXTPROVIDER_COMMANDINFORMATION_HPP
#define WIFIMAC_HELPER_CONTEXTPROVIDER_COMMANDINFORMATION_HPP

// declaration of the commands which are read by the specific command readers
#include <WIFIMAC/pathselection/BeaconLinkQualityMeasurement.hpp>
#include <WIFIMAC/pathselection/ForwardingCommand.hpp>
#include <DLL/UpperConvergence.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WNS/service/dll/Address.hpp>
#include <WIFIMAC/Layer2.hpp>

#include <WNS/probe/bus/CompoundContextProvider.hpp>
#include <WNS/ldk/CommandReaderInterface.hpp>
#include <WNS/ldk/fun/FUN.hpp>

#include <WNS/isClass.hpp>

namespace wifimac { namespace helper { namespace contextprovider {

	/** @brief Base class for all ContextProviders which use a specific command in
	 * the compound to determine the context
	 * input variables:
	 *   commandName: the name of the command where the information is readable
     *   specificKey: the name of the key which allows context-filtering
	 *                afterwards. Use a key starting with 'MAC.Compound' so that
	 *                it is clear that only compound-based probes (e.g. Packet)
	 *                can use this filter
	 */
	class CommandInformationContext:
		virtual public wns::probe::bus::CompoundContextProvider
	{
	public:
		CommandInformationContext(wns::ldk::fun::FUN* fun, std::string commandName, std::string specificKey):
			commandReader(fun->getCommandReader(commandName)),
			key(specificKey)
			{}

		~CommandInformationContext() {}

		virtual const std::string&
		getKey() const
			{
				return this->key;
			}
	protected:
		wns::ldk::CommandReaderInterface* commandReader;
		const std::string key;
    private:
        virtual void 
        doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const = 0;


	};

    /**
	 * @brief Context provider for a given compound: 1 if the compound is a
	 *   unicast transmission, 0 otherwise
	 *
	 * The status is determined by checking if the target address is valid
     *
	 */
	class IsUnicast:
		virtual public CommandInformationContext
	{
	public:
		IsUnicast(wns::ldk::fun::FUN* fun, std::string ucCommandName):
			CommandInformationContext(fun, ucCommandName, "MAC.CompoundIsUnicast")
			{}

		virtual
		~IsUnicast() {}

    private:
		virtual void
		doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
		{
            assure(compound, "Received NULL CompoundPtr");
            if(commandReader->commandIsActivated(compound->getCommandPool()))
            {
                if(commandReader->readCommand<dll::UpperCommand>(compound->getCommandPool())->peer.targetMACAddress.isValid())
                {
                    c.insertInt(this->key, 1);
                }
                else
                {
                    c.insertInt(this->key, 0);
                }
            }
		}
	};

	/**
	 * @brief Context provider for a given compound: 1 if the compound is
	 *   addressed to me, 0 otherwise
	 *
	 * The status is determined by checking if the compound is either broadcast
	 * or, if not, the targetAddress in the upperConvergenceCommand is one of my transceivers.
	 */
	class IsForMe:
		virtual public CommandInformationContext
	{
	public:
		IsForMe(wns::ldk::fun::FUN* fun_, std::string ucCommandName):
			CommandInformationContext(fun_, ucCommandName, "MAC.CompoundIsForMe"),
			fun(fun_)
			{}

		~IsForMe() {}

    private:
		virtual void
		doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
		{
            assure(compound, "Received NULL CompoundPtr");
            if(commandReader->commandIsActivated(compound->getCommandPool()))
            {
                wns::service::dll::UnicastAddress targetAddress =
                    commandReader->readCommand<dll::UpperCommand>(compound->getCommandPool())->peer.targetMACAddress;
                if(targetAddress.isValid())
                {
                    if(fun->getLayer<wifimac::Layer2*>()->isTransceiverMAC(targetAddress))
                    {
                        c.insertInt(this->key, 1);
                    }
                    else
                    {
                        c.insertInt(this->key, 0);
                    }
                }
                else
                {
                    // broadcast -> is for me
                    c.insertInt(this->key, 1);
                }
            }
		}

        wns::ldk::fun::FUN* fun;
	};

	/**
	 * @brief Context provider for a given compound: 1 if the compound is
	 *   send by me, 0 otherwise
	 *
	 * The status is determined by checking if the compound is either broadcast
	 * or, if not, the sourceAddress in the upperConvergenceCommand is one of my transceivers.
	 */
	class IsFromMe:
		virtual public CommandInformationContext
	{
	public:
		IsFromMe(wns::ldk::fun::FUN* fun_, std::string ucCommandName):
			CommandInformationContext(fun_, ucCommandName, "MAC.CompoundIsFromMe"),
			fun(fun_)
			{}

		~IsFromMe() {}

    private:
		virtual void
		doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
		{
            assure(compound, "Received NULL CompoundPtr");
            if(commandReader->commandIsActivated(compound->getCommandPool()))
            {
                wns::service::dll::UnicastAddress sourceAddress =
                    commandReader->readCommand<dll::UpperCommand>(compound->getCommandPool())->peer.sourceMACAddress;
                if(sourceAddress.isValid())
                {
                    if(fun->getLayer<wifimac::Layer2*>()->isTransceiverMAC(sourceAddress))
                    {
                        c.insertInt(this->key, 1);
                    }
                    else
                    {
                        c.insertInt(this->key, 0);
                    }
                }
                else
                {
                    // broadcast -> is not from me
                    c.insertInt(this->key, 0);
                }
            }
		}

        wns::ldk::fun::FUN* fun;
	};

	/**
	 * @brief Context provider for a given compound: Filters by the source
     *	address given in the upperConvergenceComand
	 */
	class SourceAddress:
		virtual public CommandInformationContext
	{
	public:
		SourceAddress(wns::ldk::fun::FUN* fun, std::string ucCommandName):
			CommandInformationContext(fun, ucCommandName, "MAC.CompoundSourceAddress")
			{}

		virtual
		~SourceAddress() {}

    private:
		virtual void
		doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
		{
            assure(compound, "Received NULL CompoundPtr");

			if(commandReader->commandIsActivated(compound->getCommandPool()) == true)
			{
				if(commandReader->readCommand<dll::UpperCommand>(compound->getCommandPool())->peer.sourceMACAddress.isValid())
				{
					// and if the command is activated, we add the tx
					// address to the context
					c.insertInt(this->key,
					commandReader->readCommand<dll::UpperCommand>(compound->getCommandPool())->peer.sourceMACAddress.getInteger());
				}
			}
		}
	};

	/**
	 * @brief Context provider for a given compound: Filters by the target
     *	address given in the upperConvergenceComand
	 */
	class TargetAddress:
		virtual public CommandInformationContext
	{
	public:
		TargetAddress(wns::ldk::fun::FUN* fun, std::string ucCommandName):
			CommandInformationContext(fun, ucCommandName, "MAC.CompoundTargetAddress")
			{}

		~TargetAddress() {}

    private:
		virtual void
		doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
		{
            assure(compound, "Received NULL CompoundPtr");

			if(commandReader->commandIsActivated(compound->getCommandPool()) == true)
			{
    			if(commandReader->readCommand<dll::UpperCommand>(compound->getCommandPool())->peer.targetMACAddress.isValid())
				{
					// and if the command is activated, we add the tx
					// address to the context
					c.insertInt(this->key,
					commandReader->readCommand<dll::UpperCommand>(compound->getCommandPool())->peer.targetMACAddress.getInteger());
				}
			}
		}
	};

	/**
	 * @brief Context provider for a given compound: Filters by number of hops
	 *    that the compound has travelled.
	 *
	 * The information is read from the magic.hopCount in the forwarding-Command
	 */
	class HopCount :
			virtual public CommandInformationContext
	{
	public:
		HopCount(wns::ldk::fun::FUN* fun, std::string forwardingCommandName):
			CommandInformationContext(fun, forwardingCommandName, "MAC.CompoundHopCount")
			{}

		virtual
		~HopCount() {}
    
    private:
		virtual void
		doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
		{
            assure(compound, "Received NULL CompoundPtr");                

			if(commandReader->commandIsActivated(compound->getCommandPool()) == true)
			{
				if(commandReader->
				   readCommand<wifimac::pathselection::ForwardingCommand>
				   (compound->getCommandPool())->magic.hopCount > 0)
				{
					// and if the command is activated, we add the tx
					// address to the context
					c.insertInt(this->key,
						commandReader->
						readCommand<wifimac::pathselection::ForwardingCommand>
						(compound->getCommandPool())->magic.hopCount);
				}
			}
		}
	};

	/**
	 * @brief Context provider for a given compound: Filters by the index of the
	 *    MCS with which the compound was send (will be send)
	 *
	 * The information is read from the phyUserCommand
	 */
    class DataBitsPerSymbol:
        virtual public CommandInformationContext
    {
    public:
        DataBitsPerSymbol(wns::ldk::fun::FUN* fun, std::string managerCommandName):
            CommandInformationContext(fun, managerCommandName, "MAC.CompoundDBPS")
            {}

        virtual
        ~DataBitsPerSymbol() {}

    private:
        virtual void
        doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
            {
            assure(compound, "Received NULL CompoundPtr");

            if(commandReader->commandIsActivated(compound->getCommandPool()) == true)
            {
                wifimac::convergence::PhyMode phymode = commandReader->readCommand<wifimac::lowerMAC::ManagerCommand>
                    (compound->getCommandPool())->getPhyMode();
                c.insertInt(this->key, phymode.getDataBitsPerSymbol());
            }
        }
    };

    /**
	 * @brief Context provider for a given compound: Filters by the number of
     *    spatial streams with which the compound was send (will be send)
	 *
	 * The information is read from the phyUserCommand
	 */
    class SpatialStreams :
        virtual public CommandInformationContext
    {
    public:
        SpatialStreams(wns::ldk::fun::FUN* fun, std::string managerCommandName):
            CommandInformationContext(fun, managerCommandName, "MAC.CompoundSpatialStreams")
            {}

        virtual
        ~SpatialStreams() {}

    private:
        virtual void
        doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
            {
            assure(compound, "Received NULL CompoundPtr");

            if(commandReader->commandIsActivated(compound->getCommandPool()) == true)
            {
                wifimac::convergence::PhyMode mcs = commandReader->readCommand<wifimac::lowerMAC::ManagerCommand>
                    (compound->getCommandPool())->getPhyMode();
                c.insertInt(this->key, mcs.getNumberOfSpatialStreams());
            }
        }
    };
}}}

#endif //WIFIMAC_HELPER_CONTEXTPROVIDER_COMMANDINFORMATION_HPP
