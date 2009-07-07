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

import dll.Layer2
import openwns.node

class RANG( dll.Layer2.Layer2 ):
    dllDataTransmissions = None
    dllNotifications = None
    learnAPfromIncomingData = None

    def __init__(self, node, parentLogger = None):
        super(RANG,self).__init__(node, "RANG", parentLogger)
        self.nameInComponentFactory = "dll.RANG"
        self.dllDataTransmissions = []
        self.dllNotifications = []
        self.learnAPfromIncomingData = False

    def addAP(self, ap):
        self.dllDataTransmissions.append(openwns.node.FQSN(ap, ap.dll.dataTransmission))
        self.dllNotifications.append(openwns.node.FQSN(ap, ap.dll.notification))
