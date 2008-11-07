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

from NodeCreator import NodeCreator
from ChannelManagerPool import ChannelManagerPool

from wns.PyConfig import Sealed

class Node(Sealed):
    position = None
    transceivers = None

    def __init__(self, position):
        self.transceivers = []
        self.position = position

class idGenerator:
	nextId = None
	def __init__(self):
		self.nextId = 0
	def next(self):
		self.nextId += 1
		return self.nextId
	def get(self, num):
		bunch = []
		for i in xrange(num):
			bunch.append(self.next())
		return bunch
