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
import openwns.Probe

from openwns import dBm

import wifimac.Logger

class BeaconLinkQualityMeasurementConfig(object):
    windowLength = 9.5
    noiseLevel = dBm(-94)
    meanFrameSize = 1500*8

    # variables are set globally
    maximumACKDuration = None
    slotDuration = None
    sifsDuration = None

    preambleDuration = 20E-6
    scalingFactor = 0.00144
    maxMissedBeacons = 9

    def __init__(self, **kw):
        openwns.pyconfig.attrsetter(self, kw)


class BeaconLinkQualityMeasurement(openwns.Probe.Probe):
    """ Measures the quality of existing links via the received beacons"""
    __plugin__ = "wifimac.pathselection.BeaconLinkQualityMeasurement"

    logger = None
    pathSelectionServiceName = None
    sinrMIBServiceName = None
    beaconInterval = None
    linkParams = None
    managerName = None
    phyUserCommandName = None

    receivedPowerProbeName = None
    peerMeasurementProbeName = None
    linkCostProbeName = None
    myConfig = None

    def __init__(self, fuName, commandName,
             beaconInterval,
             sinrMIBServiceName, managerName, phyUserCommandName, probePrefix,
             config,
             parentLogger=None, **kw):
        super(BeaconLinkQualityMeasurement, self).__init__(name=fuName, commandName = commandName)
        self.logger = wifimac.Logger.Logger(name = "BeaconLQM", parent = parentLogger)
        self.pathSelectionServiceName = 'PATHSELECTIONOVERVPS'
        self.sinrMIBServiceName = sinrMIBServiceName
        self.managerName = managerName
        self.beaconInterval = beaconInterval
        self.phyUserCommandName = phyUserCommandName
        self.receivedPowerProbeName = probePrefix + ".receivedPower"
        self.peerMeasurementProbeName = probePrefix + ".peerMeasurement"
        self.linkCostProbeName = probePrefix + ".linkCost"

        assert(config.__class__ == BeaconLinkQualityMeasurementConfig)
        self.myConfig = config

        openwns.pyconfig.attrsetter(self, kw)
