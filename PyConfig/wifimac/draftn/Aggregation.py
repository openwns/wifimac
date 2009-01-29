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
import openwns.Concatenation

import wifimac.Logger

class AggregationConfig(object):
    maxSize = 65535*8
    """ Maximum size in Bits of aggregated frame """
    maxEntries = 10
    """ Maximum number of entries (frames) of aggregated frame """
    maxDelay = 0.01
    """ Maximum delay that an entry waits for other entries before transmission. Only useful if aggregation is non-impatient """

    numBitsIfConcatenated = 0
    """ Number of bits for the aggregation header; IEEE 802.11n A-MPDU does not require a special aggregation header """
    numBitsIfNotConcatenated = 0
    """ Number of bits for non-aggregated frames, i.e. if the container consists of one frame only """
    numBitsPerEntry = 4*8
    """ Number of bits for each entry in the container (only if more than one entry exists) """
    entryPaddingBoundary = 4*8
    """ Each entry is padded so that its size is a multiple of this boundary """
    countPCISizeOfEntries = True
    """ Include the PCI size of each entry for the size calculation """
    impatientTransmission = False
    """ If impatient, the aggregation transmits the container as soon as possible, i.e. the lower FU is accepting.
    If not impatient, the container is transmitted only if
    a) maxEntries or maxDelay is reached (maxSize is not estimated)
    b) a ACK-frame (can also be BAreq) indicates the conclusion of the aggregation
    Impatience reduces delay, patience improves efficiency. Under saturation, impatient becomes patient.
    With impatience, BAreq might be transmitted separately from the rest!
    """

class Aggregation(openwns.Concatenation.Concatenation):

    __plugin__ = 'wifimac.lowerMAC.Aggregation'
    """ Name in FU Factory """

    myConfig = None
    managerName = None

    def __init__(self, functionalUnitName, commandName, managerName, config, parentLogger = None, **kw):
        super(Aggregation, self).__init__(maxSize = config.maxSize, maxEntries = config.maxEntries,
                                          functionalUnitName = functionalUnitName, commandName = commandName,
                                          parentLogger = parentLogger, logName = "Aggregation", moduleName = "WiFiMAC")
        self.managerName = managerName
        assert(config.__class__ == AggregationConfig)
        self.myConfig = config

        # parent class does not support the myConfig, so set variables here
        self.maxSize = self.myConfig.maxSize
        self.maxEntries = self.myConfig.maxEntries
        self.numBitsIfConcatenated = self.myConfig.numBitsIfConcatenated
        self.numBitsIfNotConcatenated = self.myConfig.numBitsIfNotConcatenated
        self.numBitsPerEntry = self.myConfig.numBitsPerEntry
        self.entryPaddingBoundary = self.myConfig.entryPaddingBoundary
        self.countPCISizeOfEntries = self.myConfig.countPCISizeOfEntries
        
        openwns.pyconfig.attrsetter(self, kw)
