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
import openwns.ARQ
import openwns.Probe
from openwns import dB

import wifimac.Logger
import wifimac.convergence.PhyMode

class BlockACKConfig(object):

    sifsDuration = None
    """ set globally in Layer2.py """
    preambleProcessingDelay = None
    """ set globally in Layer2.py """

    maximumACKDuration = None
    """ expected ACK duration for a BA is higher than the legacy ACK, default 68e-6 (set globally)"""

    blockACKBits = 32*8
    """ Size of BlockACK """

    blockACKRequestBits = 24*8
    """ Size of BlockACKrequest """

    sizeUnit = 'PDU'
    """ size unit for all size calculations (e.g. capacity, maxOnAir)"""
    capacity = 100
    """ total storage size for BlockACK FU: <sizeUnit> queued for transmissions + waiting for ACK + rx-reordering """

    maxOnAir = 10
    """ transmission window link of any link """

    maximumTransmissions = 4
    """ maximum number of transmission tries (including first one) before the frame is dropped """

    impatient = True
    """ Transmission of the BAreq can be done according to two strategies:
    a) impatient: Send BAreq as soon as no other frames are in the reception queue, i.e. after a minimum of
    one frame has been send.
    b) non-impatient: Send BAreq only if maxOnAir is exploited.
    Impatient means lower waiting time for the BA, but higher overhead for non-saturated, non-error-prone links
    """

    blockACKPhyMode = wifimac.convergence.PhyMode.IEEE80211a().getLowest()

class BlockACK(openwns.Probe.Probe):

    __plugin__ = 'wifimac.draftn.BlockACK'
    """ Name in FU Factory """

    arqStatusCollector = None

    managerName = None
    rxStartEndName = None
    txStartEndName = None
    perMIBServiceName = None

    myConfig = None
    logger = None

    # probe name
    numTxAttemptsProbeName = None

    def __init__(self, functionalUnitName, commandName, managerName, rxStartEndName, txStartEndName, perMIBServiceName, probePrefix, config, parentLogger = None, **kw):
        super(BlockACK, self).__init__(name = functionalUnitName, commandName = commandName)
        self.managerName = managerName
        self.rxStartEndName = rxStartEndName
        self.txStartEndName = txStartEndName
        self.perMIBServiceName = perMIBServiceName
        self.logger = wifimac.Logger.Logger("BlockACK", parentLogger)
        assert(config.__class__ == BlockACKConfig)
        self.myConfig = config
        self.numTxAttemptsProbeName = probePrefix + '.numTxAttempts'
        self.arqStatusCollector = openwns.ARQ.NoStatusCollection(self.logger)
        openwns.pyconfig.attrsetter(self, kw)
