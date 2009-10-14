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

#include <WIFIMAC/convergence/GuiWriter.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/ldk/fun/FUN.hpp>
#include <WNS/pyconfig/View.hpp>
#include <WNS/probe/bus/ContextProviderCollection.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>

#include <boost/tuple/tuple.hpp>


#include <sstream>

using namespace wifimac::convergence;


int GuiWriter::gui_station_counter_ = 0;


GuiWriter::GuiWriter():
    first_created(true)
{

    gui_station_id_ = gui_station_counter_;
    gui_station_counter_ ++;
}
// GuiWriter



GuiWriter::~GuiWriter()
{ }


void GuiWriter::setManagerAndFun(wifimac::lowerMAC::Manager* manager, wns::ldk::fun::FUN* fun)
{
    manager_ = manager;
    fun_ = fun;

    wns::probe::bus::ContextProviderCollection* cpcParent = 
        &fun->getLayer()->getContextProviderCollection();

    wns::probe::bus::ContextProviderCollection cpc(cpcParent);

    guiProbe_ = wns::probe::bus::ContextCollectorPtr(
        new wns::probe::bus::ContextCollector(cpc, "wifimac.guiProbe"));

}



void GuiWriter::writeToProbe(const wns::ldk::CompoundPtr& compound, wns::simulator::Time frameTxDuration)
{
    if(!guiProbe_->hasObservers())
        return;

    std::stringstream guiStr, guiStr1, guiStr2, guiStr3;

    int macaddr = manager_->getMACAddress().getInteger() -1;
    int frametype = manager_->getFrameType(compound->getCommandPool());
    unsigned int aPhyMode = manager_->getPhyMode(
        compound->getCommandPool()).getNumberOfSpatialStreams();

    int channel = 1;
    int TxPower = 200;

    if(first_created)
    {
        first_created = false;
        guiStr << wns::simulator::getEventScheduler()->getTime() * 1000000 
        << " Usecs STA: " << gui_station_id_ << " CREATED";
        guiProbe_->put(0.0, boost::make_tuple("wifimac.guiText", guiStr.str()));

        guiStr1 << wns::simulator::getEventScheduler()->getTime() * 1000000 
        << " Usecs RegistChannel: " << channel << "  S: " << gui_station_id_ 
        << "  IEEE802.11 MAC Id: " << macaddr ;
        //<< " Lx: " <<  << " Ly: " <<  << " Lz: " << ;

        guiProbe_->put(0.0, boost::make_tuple("wifimac.guiText", guiStr1.str()));
        //0.0000  Usecs STA: 0 CREATED
        //0.0000  Usecs RegistChannel: 0  S: 0   IEEE802.11 MAC Id: 0 Lx: 0.010000  Ly: 0.010000  Lz: NaN
    }


    guiStr2 << wns::simulator::getEventScheduler()->getTime() * 1000000 
    << " Usecs P: " << gui_station_id_ << "  ";

    switch(frametype)
    {
        case 1:
            guiStr2<< "T: preamble";
            break;
        case 2:
            guiStr2<< "T: data";
            break;
        case 3:
            guiStr2<< "T: data_txop";
            break;
        case 4:
            guiStr2<< "T: ack";
            break;
        case 5:
            guiStr2<< "T: beacon";
            break;
        default:
            guiStr2<< "T: unknown";
            break;

    }

    guiProbe_->put(0.0, boost::make_tuple("wifimac.guiText", guiStr2.str()));



    guiStr3 << wns::simulator::getEventScheduler()->getTime() * 1000000 
    << " Usecs C: " << channel << "  S: " << gui_station_id_ << "  L: " 
    << frameTxDuration * 1000000 << "  M: " << aPhyMode + 999 << "  P: " 
    << TxPower;

    guiProbe_->put(0.0, boost::make_tuple("wifimac.guiText", guiStr3.str()));


} // writeToProbe

