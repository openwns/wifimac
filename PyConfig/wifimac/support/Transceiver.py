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
from wns.Sealed import Sealed
import wns.PyConfig
from wns import dB, dBm

import wifimac.Layer2

class Basic(Sealed):
    frequency = None
    txPower = dBm(30)
    position = None

    layer2 = None

    def __init__(self, frequency):
        self.layer2 = wifimac.Layer2.Config(frequency)
        self.frequency = frequency

class Mesh(Basic):
    def __init__(self, frequency, forwarding):
        super(Mesh, self).__init__(frequency)
        self.layer2.forwardingEnabled = forwarding
        self.layer2.beacon.enabled = True

class Station(Basic):
    def __init__(self, frequency, position, scanFrequencies, scanDuration):
        super(Station, self).__init__(frequency)
        self.position = position
        self.layer2.forwardingEnabled = False
        self.layer2.beacon.enabled = False
        self.layer2.beacon.scanFrequencies = scanFrequencies
        self.layer2.beacon.scanDuration = scanDuration


