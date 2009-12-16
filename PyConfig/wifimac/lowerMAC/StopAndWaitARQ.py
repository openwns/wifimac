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

class StopAndWaitARQConfig(object):

    shortRetryLimit = 7
    """ rety limit of frames <= RTSThreshold """
    longRetryLimit = 4
    """ retry limit of frames >= RTSThreshold """
    rtsctsThreshold = None
    """ set globally in Layer2.py"""

    ackPhyMode = wifimac.convergence.PhyMode.IEEE80211a().getLowest()
    """ PhyMode with which the ack is transmitted --> default the lowest
        for most robustness
    """

    bianchiRetryCounter = False
    """ Retry counter is counted as in the Bianchi model, which is not
        according to the standard """

    """ Variables are set globally """
    sifsDuration = None
    maximumACKDuration = None
    ackTimeout = None

    statusCollector = openwns.ARQ.StatusCollectorTwoSizesWindowed
    windowSize = 1.0
    minSamples = 10
    insufficientSamplesReturn = -1.0
    frameSizeThreshold = 1000

    bitsPerIFrame = 0
    bitsPerRRFrame = 10*8+4*8

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
    maximumACKDuration = None
    ackTimeout = None
    ackPhyMode = None
    bianchiRetryCounter = None

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
        self.maximumACKDuration = config.maximumACKDuration
        self.ackTimeout = config.ackTimeout
        self.ackPhyMode = config.ackPhyMode
        self.bitsPerIFrame = config.bitsPerIFrame
        self.bitsPerRRFrame = config.bitsPerRRFrame
        self.bianchiRetryCounter = config.bianchiRetryCounter
        self.arqStatusCollector = config.statusCollector(self.logger)
        self.arqStatusCollector.setParams(windowSize = config.windowSize,
                                          minSamples = config.minSamples,
                                          insufficientSamplesReturn = config.insufficientSamplesReturn,
                                          frameSizeThreshold = config.frameSizeThreshold)
        self.numTxAttemptsProbeName = probePrefix + '.numTxAttempts'
        openwns.pyconfig.attrsetter(self, kw)
