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

import openwns.node
import openwns.pyconfig
import openwns.Probe

import wifimac.Logger

class Service(object):
    nameInServiceFactory  = None
    serviceName = None

class PathSelectionOverVPS(Service):
    logger  = None
    upperConvergenceName = None

    def __init__(self, serviceName, upperConvergenceName=None, parentLogger=None, **kw):
        self.nameInServiceFactory = 'wifimac.pathselection.PathSelectionOverVPS'
        self.serviceName = serviceName
        self.upperConvergenceName = upperConvergenceName
        self.logger = wifimac.Logger.Logger(name = 'PS-VPS', parent = parentLogger)
        openwns.pyconfig.attrsetter(self, kw)


class Knowledge(object):
    alpha = None
    costs = None
    readOutActive = None
    numNodes = None

    def __init__(self, alpha, numNodes):
        self.alpha = alpha
        self.readOutActive = False
        self.costs = []
        self.numNodes = numNodes

    def add(self, tx, rx, cost):
        assert(self.readOutActive == False)
        assert(tx < self.numNodes)
        assert(rx < self.numNodes)
        self.costs.append([tx, rx, cost])

    def get(self, i):
        self.readOutActive = True
        assert(i < len(self.costs))
        return(self.costs[i])

    def size(self):
        return(len(self.costs))

class VirtualPS(openwns.node.Component):
    """ The virtual path selection service hold a global knowledge about the network
        toplogy; it is feed and accessed by the PathSelectionOverVPS Compound in each
        pathselection-enabled node"""

    nameInComponentFactory = "wifimac.pathselection.VirtualPathSelection"

    logger = None
    numNodes = None
    preKnowledge = None

    def __init__(self, node, numNodes, preKnowledgeAlpha = 0.0, parentLogger=None):
        super(VirtualPS, self).__init__(node, "VPS")
        self.numNodes = numNodes
        self.preKnowledge = Knowledge(preKnowledgeAlpha, numNodes)
        self.logger = wifimac.Logger.Logger(name="VPS", parent=parentLogger)

class VirtualPSServer(openwns.node.Node, openwns.node.NoRadio):
    vps = None
    def __init__(self, name, numNodes):
        openwns.node.Node.__init__(self, name)
        self.setProperty("Type", "VPS")
        self.vps = VirtualPS(node = self, numNodes = numNodes, parentLogger=self.logger)
