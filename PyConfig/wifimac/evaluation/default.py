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
from openwns.evaluation import *

class AcceptByStationType(ITreeNodeGenerator):
    """ Provides a gate which can only be passed by probes of certain station types """
    def __init__(self, includeSTAs, includeMPs):
        self.includeSTAs = includeSTAs
        self.includeMPs = includeMPs

    def __call__(self, pathname):
        nodes = [1]
        if(self.includeSTAs):
            nodes.append(3)
        if(self.includeMPs):
            nodes.append(2)

        for treeNode in Accept(by = 'MAC.StationType', ifIn = nodes)(pathname):
            yield treeNode

class EnumerateByStationType(ITreeNodeGenerator):
    """ Provides sorting according to station type """
    def __init__(self, includeSTAs, includeMPs):
        self.includeSTAs = includeSTAs
        self.includeMPs = includeMPs

    def __call__(self, pathname):
        keys = [1]
        names = ['AP']
        if(self.includeSTAs):
            keys.append(3)
            names.append('STA')
        if(self.includeMPs):
            keys.append(2)
            names.append('MP')

        for treeNode in Enumerated(by = 'MAC.StationType', keys = keys, names = names, format = '%s')(pathname):
            yield treeNode

class SeparateByNode(ITreeNodeGenerator):

    def __init__(self, byNodeId, ids):

        self.byNodeId = byNodeId
        self.ids = ids

    def __call__(self, pathname):
        if self.byNodeId:
            for treeNode in Separate(by = 'MAC.Id', forAll = self.ids, format = 'Node%d')(pathname):
                yield treeNode
        else:
            for treeNode in Separate(by = 'MAC.TransceiverAddress', forAll = self.ids, format = 'Adr%d')(pathname):
                yield treeNode

class StationTypeToAddressTable(ITreeNodeGenerator):

    def __init__(self, sortPerNodeId, ids):
        self.sortPerNodeId = sortPerNodeId
        self.ids = ids

    def __call__(self, pathname):
        if self.sortPerNodeId:
            for treeNode in Table(axis1 = 'MAC.StationType', minValue1 = 1, maxValue1 = 3+1, resolution1 = 1,
                                  yAxis = 'MAC.Id',
                                  minYValue = min(self.ids), maxYValue = max(self.ids), yResolution = 1,
                                  values = ['mean', 'variance', 'trials'],
                                  formats = ['MatlabReadableSparse'])(pathname):
                yield treeNode
        else:
            for treeNode in Table(xAxis = 'MAC.StationType',
                                  minXValue = 1, maxXValue = 3+1, xResolution = 1,
                                  yAxis = 'MAC.TransceiverAddress',
                                  minYValue = min(self.ids), maxYValue = max(self.ids), yResolution = 1,
                                  values = ['mean', 'variance', 'trials'], formats = ['MatlabReadableSparse'])(pathname):
                yield treeNode


def installEvaluation(sim, settlingTime, apIds, mpIds, staIds, apAdrs, mpAdrs, staAdrs, maxHopCount, performanceProbes=True, networkProbes=False, draftNProbes=False, useDLRE = False):

    if(performanceProbes):
        # packet delay probe
        # * the station type determines if it is uplink (measured at the APs) or downlink (measured at the STAs) traffic
        # * the probe is measured (a) completely and (b) separated by hop count
        node = openwns.evaluation.createSourceNode(sim, 'wifimac.e2e.packet.incoming.delay')
        node.appendChildren(SettlingTimeGuard(settlingTime))
        n = node.getLeafs().appendChildren(Enumerated(by = 'MAC.StationType', keys = [1,3], names = ['ul', 'dl'], format = '%s'))
        if(useDLRE):
            node.getLeafs().appendChildren(DLRE(mode = 'g', xMin = 0.0, xMax = 0.1, intervalWidth = 0.001,
                                                name = "wifimac.e2e.packet.incoming.delay",
                                                description = "Incoming packet delay [s]"))
        else:
            node.getLeafs().appendChildren(PDF(minXValue = 0.0, maxXValue = 0.1, resolution=100,
                                               name = "wifimac.e2e.packet.incoming.delay",
                                               description = "Incoming packet delay [s]"))

        if(maxHopCount > 0):
            n.appendChildren(Separate(by = "MAC.CompoundHopCount", forAll = range(1, maxHopCount+1), format = "hc%d"))
            if(useDLRE):
                n.getLeafs().appendChildren(DLRE(mode = 'g', xMin = 0.0, xMax = 0.1, intervalWidth = 0.001,
                                                 name = "wifimac.e2e.packet.incoming.delay",
                                                 description = "Incoming packet delay [s]"))
            else:
                n.getLeafs().appendChildren(PDF(minXValue = 0.0, maxXValue = 0.1, resolution=100,
                                                name = "wifimac.e2e.packet.incoming.delay",
                                                description = "Incoming packet delay [s]"))

        # packet head-of-line delay probe measures the duration when
        # the packet leaves the queue until it is received
        # successfully. In contrast to the usual definition, the delay
        # does not include the duration for the (successful) ACK
        # transmission. E.g. if the frame is received successfully,
        # but the ACK not, the frame is re-transmitted (probably
        # several times!). This is NOT included here.
        node = openwns.evaluation.createSourceNode(sim, 'wifimac.hol.packet.incoming.delay')
        node.appendChildren(SettlingTimeGuard(settlingTime))
        if(useDLRE):
            node.getLeafs().appendChildren(DLRE(mode = 'g', xMin = 0.0, xMax = 0.1, intervalWidth = 0.001,
                                                name = "wifimac.hol.packet.delay",
                                                description = "Packet service time [s]"))
        else:
            node.getLeafs().appendChildren(PDF(minXValue = 0.0, maxXValue = 0.1, resolution=100,
                                               name = "wifimac.hol.packet.delay",
                                               description = "Packet service time [s]"))

    if(networkProbes):
        # Table probe for bit throughput, separated by nodeId
        for sourceName in ['wifimac.e2e.window.incoming.bitThroughput.hop', 'wifimac.e2e.window.aggregated.bitThroughput.hop']:
            node = openwns.evaluation.createSourceNode(sim, sourceName)
            node.appendChildren(SettlingTimeGuard(settlingTime))
            node.getLeafs().appendChildren(Accept(by = 'MAC.StationType', ifIn = [3]))
            minId = min(staIds)
            maxId = max(staIds)
            node.getLeafs().appendChildren(Table(axis1 = 'MAC.Id', minValue1 = minId, maxValue1 = maxId+1, resolution1 = maxId+1 - minId,
                                                 values = ['mean', 'trials'],
                                                 formats = ['MatlabReadableSparse']))

    if(networkProbes):
        #  * Unicast & received by me
        #  * Table with source | target | MCS
        for sourceName in ['wifimac.linkQuality.msduSuccessRate', 'wifimac.linkQuality.sinr']:
            node = openwns.evaluation.createSourceNode(sim, sourceName)
            node.appendChildren(SettlingTimeGuard(settlingTime))
            node.getLeafs().appendChildren(Accept(by = 'MAC.CompoundIsForMe', ifIn = [1]))
            node.getLeafs().appendChildren(Accept(by = 'MAC.CompoundIsUnicast', ifIn = [1]))
            # limit to aps/mps on demand
            # node.getLeafs().appendChildren(Accept(by = 'MAC.StationType', ifIn = [1, 2]))
            # node.getLeafs().appendChildren(Accept(by = 'MAC.CompoundSourceAddress', ifIn = apAdrs + mpAdrs))
            # node.getLeafs().appendChildren(Accept(by = 'MAC.CompoundTargetAddress', ifIn = apAdrs + mpAdrs))
            minAdr = min(apAdrs + mpAdrs + staAdrs)
            maxAdr = max(apAdrs + mpAdrs + staAdrs)
            node.getLeafs().appendChildren(Table(axis1 = 'MAC.CompoundSourceAddress', minValue1 = minAdr, maxValue1 = maxAdr+1, resolution1 = maxAdr+1-minAdr,
                                                 axis2 = 'MAC.CompoundTargetAddress', minValue2 = minAdr, maxValue2 = maxAdr+1, resolution2 = maxAdr+1-minAdr,
                                                 axis3 = 'MAC.CompoundSpatialStreams', minValue3 = 1, maxValue3 = 4, resolution3 = 3,
                                                 axis4 = 'MAC.CompoundMCS', minValue4 = 1, maxValue4 = 9, resolution4 = 8,
                                                 values = ['mean', 'trials', 'variance'],
                                                 formats = ['MatlabReadableSparse']))

        # * Unicast & transmitted by me
        # * MCS is not yet set --> table with source | target
        probeNames = ['wifimac.linkQuality.numTxAttempts']
        if draftNProbes:
            # only for draftN probe evaluation
            probeNames.append('wifimac.aggregation.sizeFrames')
        for sourceName in probeNames:
            node = openwns.evaluation.createSourceNode(sim, sourceName)
            node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime))
            #node.getLeafs().appendChildren(Logger())
            node.getLeafs().appendChildren(Accept(by = 'MAC.CompoundIsUnicast', ifIn = [1]))
            # limit to APs/MPs on demand
            # node.getLeafs().appendChildren(Accept(by = 'MAC.StationType', ifIn = [1, 2]))
            # node.getLeafs().appendChildren(Accept(by = 'MAC.CompoundSourceAddress', ifIn = apAdrs + mpAdrs))
            # node.getLeafs().appendChildren(Accept(by = 'MAC.CompoundTargetAddress', ifIn = apAdrs + mpAdrs))
            minAdr = min(apAdrs + mpAdrs + staAdrs)
            maxAdr = max(apAdrs + mpAdrs + staAdrs)
            node.getLeafs().appendChildren(Table(axis1 = 'MAC.CompoundSourceAddress', minValue1 = minAdr, maxValue1 = maxAdr+1, resolution1 = maxAdr+1-minAdr,
                                                 axis2 = 'MAC.CompoundTargetAddress', minValue2 = minAdr, maxValue2 = maxAdr+1, resolution2 = maxAdr+1-minAdr,
                                                 values = ['mean', 'trials', 'variance'],
                                                 formats = ['MatlabReadableSparse']))

        # * Probes that are not packet related --> no source or target to test on!
        # * Table with Transceiver Address
        for sourceName in ['wifimac.txUpper.window.outgoing.bitThroughput', 'wifimac.channelState.busy', 'wifimac.txop.duration']:
            node = openwns.evaluation.createSourceNode(sim, sourceName)
            node.appendChildren(SettlingTimeGuard(settlingTime))
            # limit to APs/MPs on demand
            # node.getLeafs().appendChildren(Accept(by = 'MAC.StationType', ifIn = [1, 2]))
            # node.getLeafs().appendChildren(Accept(by = 'MAC.TransceiverAddress', ifIn = apAdrs + mpAdrs))
            minAdr = min(apAdrs + mpAdrs + staAdrs)
            maxAdr = max(apAdrs + mpAdrs + staAdrs)
            node.getLeafs().appendChildren(Table(axis1 = 'MAC.TransceiverAddress', minValue1 = minAdr, maxValue1 = maxAdr+1, resolution1 = maxAdr+1-minAdr,
                                                 values = ['mean', 'trials', 'variance'],
                                                 formats = ['MatlabReadableSparse']))
