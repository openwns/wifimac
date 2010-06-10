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
import wifimac.lowerMAC

class RTSCTSwithFLAConfig(object):
    # Variables are set globally in Layer2.py
    sifsDuration = None
    maximumACKDuration = None
    maximumCTSDuration = None
    preambleProcessingDelay = None
    ctsTimeout = None

    rtsctsPhyMode = wifimac.convergence.PhyMode.IEEE80211a().getLowest()
    """ PhyMode with which the rts and cts are transmitted --> default the lowest for most robustness"""

    # sizes include the CRC
    rtsBits = 16*8 + 4*8
    ctsBits = 10*8 + 4*8

    rtsctsThreshold = None
    """ Set globally in Layer2.py """

    rtsctsOnTxopData = False
    estimatedValidity = 100e-6

class RTSCTSwithFLA(wifimac.lowerMAC.RTSCTS):

    __plugin__ = 'wifimac.draftn.RTSCTSwithFLA'
    """ Name in FU Factory """

    sinrMIBServiceName = None
    raName = None

    def __init__(self, functionalUnitName, commandName, raName, managerName, sinrMIBServiceName, protocolCalculatorName, phyUserName, arqName, navName, rxStartName, txStartEndName, probePrefix, config, parentLogger = None, **kw):
        super(RTSCTSwithFLA, self).__init__(functionalUnitName, commandName, managerName, protocolCalculatorName, phyUserName, arqName, navName, rxStartName, txStartEndName, probePrefix, config, parentLogger)

	self.raName = raName
	self.sinrMIBServiceName = sinrMIBServiceName

        openwns.pyconfig.attrsetter(self, kw)
