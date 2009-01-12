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
import openwns.Probe

################################
# Special Probes for the WiFiMAC
################################

class WindowProbeBus(openwns.Probe.WindowProbeBus):
    """ Special WiFiMAC Window-Probe which allows for
         * Filtering by the context MAC.WindowProbeHopCount
         * Sorting by all contexts which are defined in the manager
            -> Sorting by transceiver type
    """
    __plugin__ = 'wifimac.helper.HopContextWindowProbe'

    """ Names of the associated probes """
    incomingHopCountedBitThroughputProbeName = None
    incomingHopCountedCompoundThroughputProbeName = None
    aggregatedHopCountedBitThroughputProbeName = None
    aggregatedHopCountedCompoundThroughputProbeName = None

    """ Required to read the hop-count """
    forwardingCommandName = None

    def __init__(self, name, prefix, commandName, forwardingCommandName, windowSize = 1.0, sampleInterval = None, parentLogger = None, moduleName = 'WiFiMAC', **kw):
        super(openwns.Probe.WindowProbeBus, self).__init__(name, prefix, commandName, windowSize, sampleInterval, parentLogger, moduleName, **kw)
        self.forwardingCommandName = forwardingCommandName

        self.incomingHopCountedBitThroughputProbeName = prefix + ".window.incoming.bitThroughput.hop"
        self.incomingHopCountedCompoundThroughputProbeName = prefix + ".window.incoming.compoundThroughput.hop"
        self.aggregatedHopCountedBitThroughputProbeName = prefix + ".window.aggregated.bitThroughput.hop"
        self.aggregatedHopCountedCompoundThroughputProbeName =  prefix + ".window.aggregated.compoundThroughput.hop"
