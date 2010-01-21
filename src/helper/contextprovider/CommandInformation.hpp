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

#include <WNS/probe/bus/CommandContextProvider.hpp>

//#include <WNS/isClass.hpp>

namespace wifimac { namespace helper { namespace contextprovider {


    /**
	 * @brief Context provider for a given compound: 1 if the compound is a
	 *   unicast transmission, 0 otherwise
	 *
	 * The status is determined by checking if the target address is valid
     *
	 */
    class IsUnicast:
        virtual public wns::probe::bus::CommandContextProvider<dll::UpperCommand>
    {
    public:
        IsUnicast(wns::ldk::fun::FUN* fun, std::string ucCommandName):
            wns::probe::bus::CommandContextProvider<dll::UpperCommand>(fun, ucCommandName, "MAC.CompoundIsUnicast")
            {};
    private:
        virtual void
        doVisitCommand(wns::probe::bus::IContext& c, const dll::UpperCommand* command) const
            {
                if(command->peer.targetMACAddress.isValid())
                {
                    c.insertInt(getKey(), 1);
                }
                else
                {
                    c.insertInt(getKey(), 0);
                }
            };
    };

    /**
	 * @brief Context provider for a given compound: 1 if the compound is
	 *   addressed to me, 0 otherwise
	 *
	 * The status is determined by checking if the compound is either broadcast
	 * or, if not, the targetAddress in the upperConvergenceCommand is one of my
	 * transceivers.
	 */
    class IsForMe:
        virtual public wns::probe::bus::CommandContextProvider<dll::UpperCommand>
    {
    public:
        IsForMe(wns::ldk::fun::FUN* fun_, std::string ucCommandName):
            wns::probe::bus::CommandContextProvider<dll::UpperCommand>(fun_, ucCommandName, "MAC.CompoundIsForMe"),
            fun(fun_)
            {};

    private:
        virtual void
        doVisitCommand(wns::probe::bus::IContext& c, const dll::UpperCommand* command) const
            {
                wns::service::dll::UnicastAddress targetAddress = command->peer.targetMACAddress;
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
            };
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
        virtual public wns::probe::bus::CommandContextProvider<dll::UpperCommand>
    {
    public:
        IsFromMe(wns::ldk::fun::FUN* fun_, std::string ucCommandName):
            wns::probe::bus::CommandContextProvider<dll::UpperCommand>(fun_, ucCommandName, "MAC.CompoundIsFromMe"),
            fun(fun_)
            {};

    private:
        virtual void
        doVisitCommand(wns::probe::bus::IContext& c, const dll::UpperCommand* command) const
            {
                wns::service::dll::UnicastAddress sourceAddress = command->peer.sourceMACAddress;
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
            };
        wns::ldk::fun::FUN* fun;
    };

    /**
	 * @brief Context provider for a given compound: Filters by the source
     *	address given in the upperConvergenceComand
	 */
    class SourceAddress:
        virtual public wns::probe::bus::CommandContextProvider<dll::UpperCommand>
    {
    public:
        SourceAddress(wns::ldk::fun::FUN* fun, std::string ucCommandName):
            wns::probe::bus::CommandContextProvider<dll::UpperCommand>(fun, ucCommandName, "MAC.CompoundSourceAddress")
            {};

    private:
        virtual void
        doVisitCommand(wns::probe::bus::IContext& c, const dll::UpperCommand* command) const
            {
                if(command->peer.sourceMACAddress.isValid())
                {
                    // if the command is activated, we add the tx address to the
                    // context
                    c.insertInt(this->key,
                                command->peer.sourceMACAddress.getInteger());
                }
            }
    };

    /**
	 * @brief Context provider for a given compound: Filters by the target
     *	address given in the upperConvergenceComand
	 */
    class TargetAddress:
        virtual public wns::probe::bus::CommandContextProvider<dll::UpperCommand>
    {
    public:
        TargetAddress(wns::ldk::fun::FUN* fun, std::string ucCommandName):
            wns::probe::bus::CommandContextProvider<dll::UpperCommand>(fun, ucCommandName, "MAC.CompoundTargetAddress")
            {};

    private:
        virtual void
        doVisitCommand(wns::probe::bus::IContext& c, const dll::UpperCommand* command) const
            {
                if(command->peer.targetMACAddress.isValid())
                {
                    // if the command is activated, we add the tx address to the
                    // context
                    c.insertInt(this->key,
                                command->peer.targetMACAddress.getInteger());
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
        virtual public wns::probe::bus::CommandContextProvider<wifimac::pathselection::ForwardingCommand>
    {
    public:
        HopCount(wns::ldk::fun::FUN* fun, std::string forwardingCommandName):
            wns::probe::bus::CommandContextProvider<wifimac::pathselection::ForwardingCommand>(fun, forwardingCommandName, "MAC.CompoundHopCount")
            {};
    private:
        virtual void
        doVisitCommand(wns::probe::bus::IContext& c, const wifimac::pathselection::ForwardingCommand* command) const
            {
                if(command->magic.hopCount > 0)
                {
                    // if the command is activated, we add the tx address to the
                    // context
                    c.insertInt(this->key,
                                command->magic.hopCount);
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
        virtual public wns::probe::bus::CommandContextProvider<wifimac::lowerMAC::ManagerCommand>
    {
    public:
        DataBitsPerSymbol(wns::ldk::fun::FUN* fun, std::string managerCommandName):
            wns::probe::bus::CommandContextProvider<wifimac::lowerMAC::ManagerCommand>(fun, managerCommandName, "MAC.CompoundDBPS")
            {};

    private:
        virtual void
        doVisitCommand(wns::probe::bus::IContext& c, const wifimac::lowerMAC::ManagerCommand* command) const
            {
                wifimac::convergence::PhyMode phymode = command->getPhyMode();
                c.insertInt(this->key, phymode.getDataBitsPerSymbol());
            }
    };

    /**
	 * @brief Context provider for a given compound: Filters by the number of
     *    spatial streams with which the compound was send (will be send)
	 *
	 * The information is read from the phyUserCommand
	 */
    class SpatialStreams :
        virtual public wns::probe::bus::CommandContextProvider<wifimac::lowerMAC::ManagerCommand>
    {
    public:
        SpatialStreams(wns::ldk::fun::FUN* fun, std::string managerCommandName):
            wns::probe::bus::CommandContextProvider<wifimac::lowerMAC::ManagerCommand>(fun, managerCommandName, "MAC.CompoundSpatialStreams")
            {};

    private:
        virtual void
        doVisitCommand(wns::probe::bus::IContext& c, const wifimac::lowerMAC::ManagerCommand* command) const
            {
                wifimac::convergence::PhyMode mcs = command->getPhyMode();
                c.insertInt(this->key, mcs.getNumberOfSpatialStreams());
            }
    };
}}}

#endif //WIFIMAC_HELPER_CONTEXTPROVIDER_COMMANDINFORMATION_HPP
