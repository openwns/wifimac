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

import ofdmaphy.OFDMAPhy
import rise.Scenario

class ChannelManagerPool:
    managers = None
    bssManager = None
    channel2Manager = None
    node2Managers = None
    scenario = None

    next = None

    def __init__(self, scenario, numMeshChannels, ofdmaPhyConfig):
        self.managers = []
        for r in xrange(numMeshChannels):
            sys = ofdmaphy.OFDMAPhy.OFDMASystem('ofdma-'+str(r))
            sys.Scenario = scenario
            self.managers.append(sys)
        self.bssManager = ofdmaphy.OFDMAPhy.OFDMASystem('ofdma-bss')
        self.bssManager.Scenario = scenario

        ofdmaPhyConfig.systems.extend(self.managers)
        ofdmaPhyConfig.systems.append(self.bssManager)

        self.channel2Manager = {}
        self.node2Managers = {}
        self.next = 0

    def getManager(self, channel, nodeId):
        """ Returns the manager for the channel/nodeId, assigns
            one if none is assigned yet"""
        if(not self.node2Managers.has_key(nodeId)):
            self.node2Managers[nodeId] = []

        if(self.channel2Manager.has_key(channel)):
            # Channel is already bound to a manager, assure that the node gets this one
            manager = self.channel2Manager[channel]
            if(self.node2Managers[nodeId].count(manager) > 1):
                raise AssertionError, "Found nodeId with count(manager) > 1"
            if(self.node2Managers[nodeId].count(manager) == 0):
                self.node2Managers[nodeId].append(manager)
            return(manager)
        else:
            # Channel is unbound -> create manager which is not bound to that node
            manager = self.managers[self.next]
            self.next += 1
            self.channel2Manager[channel] = manager
            self.node2Managers[nodeId].append(manager)
            return(manager)

    def getBSSManager(self):
        return(self.bssManager)

    def nodeId2Channels(self, nodeId):
        channels = []
        if(not self.node2Managers.has_key(nodeId)):
            return channels
        else:
            for m in self.node2Managers[nodeId]:
                # find the channels for m
                for c in self.channels2Managers.keys:
                    if self.channels2Managers(c) == m:
                        channels.append(c)
            return channels
