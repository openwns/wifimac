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
#
# WiFiMac-specific filter for use by dll::CompoundSwitch
#

import dll.CompoundSwitch

class FrameType(dll.CompoundSwitch.Filter):
    __plugin__ = "wifimac.helper.FilterFrameType"
    commandName = None
    fType = None

    def __init__(self, fType, commandName, **kw):
        super(FrameType, self).__init__('Type '+fType)
        self.commandName = commandName
        self.fType = fType

class Size(dll.CompoundSwitch.Filter):
    __plugin__ = "wifimac.helper.FilterSize"
    maxSize = None
    minSize = None

    def __init__(self, minSize, maxSize, **kw):
        super(Size, self).__init__('Size '+str(minSize)+"..."+str(maxSize))
        self.maxSize = maxSize
        self.minSize = minSize

