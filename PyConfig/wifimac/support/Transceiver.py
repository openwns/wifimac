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

import openwns.pyconfig
from openwns import dBm, dB
from wifimac.lowerMAC.RateAdaptation import OpportunisticwithMIMO

import wifimac.Layer2
import wifimac.draftn

# begin example "wifimac.pyconfig.support.transceiver.Basic"
class Basic(object):
    frequency = None
    txPower = dBm(30)
    position = None

    layer2 = None

    def __init__(self, frequency):
        self.layer2 = wifimac.Layer2.Config(frequency)
        self.frequency = frequency
# end example

# begin example "wifimac.pyconfig.support.transceiver.Mesh"
class Mesh(Basic):
    def __init__(self, frequency):
        super(Mesh, self).__init__(frequency)
        self.layer2.beacon.enabled = True
# end example

class Station(Basic):
    def __init__(self, frequency, position, scanFrequencies, scanDuration):
        super(Station, self).__init__(frequency)
        self.position = position
        self.layer2.beacon.enabled = False
        self.layer2.beacon.scanFrequencies = scanFrequencies
        self.layer2.beacon.scanDuration = scanDuration

class DraftN(Basic):
    def __init__(self, frequency, numAntennas, maxAggregation, mimoCorrelation = 0.0):
        super(DraftN, self).__init__(frequency)
        self.layer2.funTemplate = wifimac.draftn.FUNTemplate
        self.layer2.phyUser.phyModesDeliverer = wifimac.draftn.PhyModes()
        self.layer2.phyUser.mimoCorrelation = mimoCorrelation
        self.layer2.maximumACKDuration = 68E-6
        self.layer2.ra.raStrategy = OpportunisticwithMIMO()
        self.layer2.manager.numAntennas = numAntennas

        # resource usage strategy: Buffer, blockACK, aggregation, TxOP
        self.layer2.bufferSize = 50
        self.layer2.blockACK.capacity = 50
        self.layer2.blockACK.maxOnAir = maxAggregation
        self.layer2.blockACK.impatient = True
        self.layer2.aggregation.maxEntries = maxAggregation
        self.layer2.aggregation.maxDelay = 1.0
        self.layer2.aggregation.impatient = False
        self.layer2.txop.txopLimit = 0.0

class DraftNStation(DraftN):
    def __init__(self, frequency, position, scanFrequencies, scanDuration, numAntennas, maxAggregation, mimoCorrelation = 0.0):
        super(DraftNStation, self).__init__(frequency, numAntennas, maxAggregation, mimoCorrelation)

        self.position = position
        self.layer2.beacon.enabled = False
        self.layer2.beacon.scanFrequencies = scanFrequencies
        self.layer2.beacon.scanDuration = scanDuration
