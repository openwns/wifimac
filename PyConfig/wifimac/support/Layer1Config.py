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
from openwns import dB
from ofdmaphy.Receiver import *

class BaseClass(object):
    receiverNoiseFigure = None
    frequency = None
    txPower = None
    bandwidth = None
    numAntennas = None
    mimoProcessing = None

class Basic(BaseClass):
    def __init__(self, frequency, txPower):
        self.frequency = frequency
        self.txPower = txPower

        self.bandwidth = 20
        self.numAntennas = 1
        self.mimoProcessing = NoCorrelationZF()
        self.receiverNoiseFigure = dB(5)

class IMTAMIMO_BS(Basic):
    def __init__(self, frequency, txPower, numAntennas):
        super(IMTAMIMO_BS, self).__init__(frequency, txPower)

        self.numAntennas = numAntennas
        self.mimoProcessing = CorrelatedStaticZF_IMTAUMi_BS(randomSpread = False, randomOrientation = False, arrayOrientation = 0.0)


class IMTAMIMO_UT(Basic):
    def __init__(self, frequency, txPower, numAntennas):
        super(IMTAMIMO_UT, self).__init__(frequency, txPower)

        self.numAntennas = numAntennas
        self.mimoProcessing = CorrelatedStaticZF_IMTAUMi_UT(randomSpread = False, randomOrientation = False, arrayOrientation = 0.0)

