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

class SeparateByNodeId(ITreeNodeGenerator):

    def __init__(self, ids):
        self.ids = ids

    def __call__(self, pathname):
        for treeNode in Separate(by = 'wns.node.Node.id', forAll = self.ids, format = 'Node%d')(pathname):
            yield treeNode

def getTop(sim, sourceName, settlingTime, nodeIds):
    node = openwns.evaluation.createSourceNode(sim, sourceName )
    node.appendChildren(SettlingTimeGuard(settlingTime))
    node.getLeafs().appendChildren(SeparateByNodeId(nodeIds))
    return node

def installEvaluation(sim, staIds, rangId, settlingTime, maxPacketDelay, maxBitThroughput, resolution=100, useDLRE = False):

    for sourceName in ['ip.endToEnd.packet.incoming.delay', 'ip.endToEnd.packet.outgoing.delay']:
        node = getTop(sim, sourceName, settlingTime, [rangId])
        if(useDLRE):
            node.getLeafs().appendChildren(DLRE(mode = 'g', xMin = 0.0, xMax = maxPacketDelay, intervalWidth = 0.1/resolution,
                                                name = sourceName,
                                                description = "End-to-End Packet Delay [s]"))
        else:
            node.getLeafs().appendChildren(PDF(minXValue = 0.0, maxXValue = maxPacketDelay, resolution = resolution,
                                               name = sourceName,
                                               description = 'End-to-End Packet Delay [s]'))

    for direction in ['incoming', 'outgoing', 'aggregated']:
        sourceName = 'ip.%s.window.%s.bitThroughput' % ('endToEnd', direction)

        # bitThroughput of the RANG as moments probe
        node = openwns.evaluation.createSourceNode(sim, sourceName )
        node.appendChildren(SettlingTimeGuard(settlingTime))
        node.getLeafs().appendChildren(Accept(by = 'wns.node.Node.id', ifIn = [rangId]))
        node.getLeafs().appendChildren(Moments(name = sourceName,
                                               description = "Throughput (" + direction + ") [bit/s]"))

        # bit throughput of each client as table probe
        node = openwns.evaluation.getSourceNode(sim, sourceName)
        minId = min(staIds)
        maxId = max(staIds)
        n = node.appendChildren(SettlingTimeGuard(settlingTime))
        n.getLeafs().appendChildren(Accept(by = 'MAC.StationType', ifIn = [2,3]))
        n.getLeafs().appendChildren(Table(axis1 = 'MAC.Id', minValue1 = minId, maxValue1 = maxId+1, resolution1 = maxId+1 - minId,
                                             values = ['mean', 'trials'],
                                             formats = ['MatlabReadableSparse']))
