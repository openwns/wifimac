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

import wifimac.Logger

class StopAndWaitARQConfig(object):

    shortRetryLimit = 7
    """ rety limit of frames <= RTSThreshold """
    longRetryLimit = 4
    """ retry limit of frames >= RTSThreshold """
    rtsctsThreshold = 3000*8

    ackPhyModeId = 0

    sifsDuration = 16E-6
    expectedACKDuration = 44E-6
    preambleProcessingDelay = 21E-6

    statusCollector = openwns.ARQ.StatusCollectorTwoSizesWindowed
    windowSize = 1.0
    minSamples = 10
    insufficientSamplesReturn = -1.0
    frameSizeThreshold = 1000

    bitsPerIFrame = 0
    bitsPerRRFrame = 10*8

class StopAndWaitARQ(openwns.Probe.Probe):
    """ Special version of StopAndWait for CSMACAMAC, includes StatusCollection via
        TwoSizesWindowed by default
    """

    __plugin__ = 'wifimac.lowerMAC.StopAndWaitARQ'
    """ Name in FU Factory """

    logger = None

    csName = None
    """ The arq needs to know the current channel state """
    rxStartName = None
    """ ... and the rxStartIndication for the ACK """
    txStartEndName = None

    shortRetryLimit = None
    longRetryLimit = None
    rtsctsThreshold = None
    sifsDuration = None
    expectedACKDuration = None
    preambleProcessingDelay = None
    ackPhyModeId = None

    arqStatusCollector = None
    functionalUnitName = None
    commandName = None
    managerName = None
    perMIBServiceName = None

    # without CRC
    bitsPerIFrame = None
    bitsPerRRFrame = None
    useSuspendProbe = False
    suspendProbeName = "timeBufferEmpty"
    resendTimeout = 0.1

    # probe
    numTxAttemptsProbeName = None

    def __init__(self, fuName, commandName, managerName, csName, rxStartName, txStartEndName, perMIBServiceName, probePrefix, config, parentLogger = None, **kw):
        super(StopAndWaitARQ, self).__init__(name=fuName, commandName=commandName)
        self.managerName = managerName
        self.csName = csName
        self.rxStartName = rxStartName
        self.txStartEndName = txStartEndName
        self.perMIBServiceName = perMIBServiceName
        self.logger = wifimac.Logger.Logger("ARQ", parentLogger)
        assert(config.__class__ == StopAndWaitARQConfig)
        self.shortRetryLimit = config.shortRetryLimit
        self.longRetryLimit = config.longRetryLimit
        self.rtsctsThreshold = config.rtsctsThreshold
        self.sifsDuration = config.sifsDuration
        self.expectedACKDuration = config.expectedACKDuration
        self.preambleProcessingDelay = config.preambleProcessingDelay
        self.ackPhyModeId = config.ackPhyModeId
        self.bitsPerIFrame = config.bitsPerIFrame
        self.bitsPerRRFrame = config.bitsPerRRFrame
        self.arqStatusCollector = config.statusCollector(self.logger)
        self.arqStatusCollector.setParams(windowSize = config.windowSize,
            minSamples = config.minSamples,
            insufficientSamplesReturn = config.insufficientSamplesReturn,
            frameSizeThreshold = config.frameSizeThreshold)
        self.numTxAttemptsProbeName = probePrefix + '.numTxAttempts'
        openwns.pyconfig.attrsetter(self, kw)
