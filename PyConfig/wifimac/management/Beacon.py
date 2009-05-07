###############################################################################
# This file is part of openWNS (open Wireless Network Simulator)
# _____________________________________________________________________________
#
# Copyright (C) 2004-2008
# Chair of Communication Networks (ComNets)
# Kopernikusstr. 16, D-52074 Aachen, Germany
# phone: ++49-241-80-27910,
# fax: ++49-241-80-22242
# email: info@openwns.org
# www: http://www.openwns.org
# _____________________________________________________________________________
#
# openWNS is free software; you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License version 2 as published by the
# Free Software Foundation;
#
# openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

import openwns.FUN
import openwns.pyconfig
from openwns import dB

import wifimac.Logger
import wifimac.convergence.PhyMode

# begin example "wifimac.pyconfig.layer2.management.beacon.beaconconfig"
class BeaconConfig(object):
    enabled = True
    delay = 0.1
    """ Initial start delay"""
    period = 0.1
    """ Beacon transmission period, common value is 100ms """
    scanDuration = 0.3
    """ duration of scanning(for each frequency) before association for STAs """
    scanFrequencies = None
    """ freqencies to scan for beacons """
    beaconPhyMode = wifimac.convergence.PhyMode.IEEE80211a().getLowest()
    """ PhyMode with which the beacon is transmitted --> default the lowest for most robustness"""
    bssId = 'ComNetsWLAN'
    """ for APs: bssId set in the beacon; for STAs: Only associate to bssIds with this name """


    def __init__(self, **kw):
        self.scanFrequencies = []
        openwns.pyconfig.attrsetter(self, kw)
# end example

class Beacon(openwns.FUN.FunctionalUnit):
    __plugin__ = 'wifimac.management.Beacon'

    logger = None
    phyUserCommandName = None
    managerName = None
    myConfig = None

    def __init__(self, functionalUnitName, commandName, managerName, phyUserCommandName, config, parentLogger = None, **kw):
        super(Beacon, self).__init__(functionalUnitName = functionalUnitName, commandName=commandName)
        self.logger = wifimac.Logger.Logger(name = "Beacon", parent = parentLogger)
        self.managerName = managerName
        self.phyUserCommandName = phyUserCommandName
        assert(config.__class__ == BeaconConfig)
        self.myConfig = config
        openwns.pyconfig.attrsetter(self, kw)
