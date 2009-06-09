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

#ifndef WIFIMAC_HELPER_CONTEXTPROVIDER_COMPOUNDSIZE_HPP
#define WIFIMAC_HELPER_CONTEXTPROVIDER_COMPOUNDSIZE_HPP

#include <WNS/probe/bus/CompoundContextProvider.hpp>
#include <WNS/ldk/CommandReaderInterface.hpp>
#include <WNS/ldk/fun/FUN.hpp>

namespace wifimac { namespace helper { namespace contextprovider {

    class CompoundSize:
        virtual public wns::probe::bus::CompoundContextProvider
    {
    public:
        CompoundSize(std::string specificKey) :
            key(specificKey)
            {};
        ~CompoundSize() {};

        virtual const std::string&
        getKey() const
            {
                return this->key;
            }
    protected:
        const std::string key;
    private:
        virtual void 
        doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const = 0;
    };

    class CompleteLengthInBits:
        virtual public CompoundSize
    {
    public:
        CompleteLengthInBits():
            CompoundSize("MAC.CompleteLengthInBits")
            {};

        virtual
        ~CompleteLengthInBits() {};

    private:
        void
        doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
            {
                Bit commandPoolSize;
                Bit dataSize;
                compound->getCommandPool()->calculateSizes(commandPoolSize, dataSize);
                c.insertInt(this->key, commandPoolSize + dataSize);
            };
    };

    class CommandPoolLengthInBits:
        virtual public CompoundSize
    {
    public:
        CommandPoolLengthInBits():
            CompoundSize("MAC.CommandPoolLengthInBits")
            {};

        virtual
        ~CommandPoolLengthInBits() {};

    private:
        void
        doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
            {
                Bit commandPoolSize;
                Bit dataSize;
                compound->getCommandPool()->calculateSizes(commandPoolSize, dataSize);
                c.insertInt(this->key, commandPoolSize);
            };
    };

    class DataLengthInBits:
        virtual public CompoundSize
    {
    public:
        DataLengthInBits():
            CompoundSize("MAC.DataLengthInBits")
            {};

        virtual
        ~DataLengthInBits() {};

    private:
        void
        doVisit(wns::probe::bus::IContext& c, const wns::ldk::CompoundPtr& compound) const
            {
                Bit commandPoolSize;
                Bit dataSize;
                compound->getCommandPool()->calculateSizes(commandPoolSize, dataSize);
                c.insertInt(this->key, dataSize);
            };
    };

}}}

#endif //WIFIMAC_HELPER_CONTEXTPROVIDER_COMPOUNDSIZE_HPP
