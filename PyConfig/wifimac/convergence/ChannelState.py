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
import openwns.Probe
import openwns.pyconfig

import wifimac.Logger

from openwns import dBm

class ChannelStateConfig(object):
    useRawEnergyDetection = False
    usePhyCarrierSense = True
    usePhyPacketLength = True
    useNAV = True
    useOwnTx = True
    useOwnRx = True

    rawEnergyThreshold = dBm(-62)
    phyCarrierSenseThreshold = dBm(-82)

    """ To probe the channel busy fraction """
    channelBusyFractionMeasurementPeriod = 0.5

    """ Variables are set globally and copied here"""
    sifsDuration = None
    preambleProcessingDelay = None
    slotDuration = None
    maximumCTSDuration = None

class ChannelState(openwns.Probe.Probe):
    __plugin__ = 'wifimac.convergence.ChannelState'

    logger = None

    myConfig = None

    managerName = None
    phyUserCommandName = None
    rtsctsCommandName = None
    txStartEndName = None
    rxStartEndName = None

    """ To probe the channel busy fraction """
    busyFractionProbeName = None

    def __init__(self, name, commandName, managerName, phyUserCommandName, rtsctsCommandName, txStartEndName, rxStartEndName, probePrefix, config, parentLogger = None, **kw):
        super(ChannelState, self).__init__(name=name, commandName=commandName)
        self.logger = wifimac.Logger.Logger(name = "ChannelState", parent = parentLogger)
        assert(config.__class__ == ChannelStateConfig)
        self.myConfig = config
        self.managerName = managerName
        self.phyUserCommandName = phyUserCommandName
        self.rtsctsCommandName = rtsctsCommandName
        self.txStartEndName = txStartEndName
        self.rxStartEndName = rxStartEndName
        self.busyFractionProbeName = probePrefix + ".busy"

        openwns.pyconfig.attrsetter(self, kw)


