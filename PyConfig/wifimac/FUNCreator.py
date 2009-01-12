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

import wifimac.convergence
import wifimac.draftn
import wifimac.lowerMAC
import wifimac.pathselection
import wifimac.management

class FUNCreator(object):
	logger = None

	transceiverAddress = None
	probeLocalIDs = None
        names = None

	def __init__(self,
		     logger,
		     transceiverAddress,
		     upperConvergenceName):

		self.logger = logger
		self.transceiverAddress = transceiverAddress

		self.probeLocalIDs = {}
		self.probeLocalIDs['MAC.TransceiverAddress'] = transceiverAddress

		self.names = dict()
		self.names['upperConvergence'] = upperConvergenceName
                self.names.update(wifimac.convergence.names)
		self.names.update(wifimac.pathselection.names)
		self.names.update(wifimac.management.names)
		self.names.update(wifimac.lowerMAC.names)
		self.names.update(wifimac.draftn.names)

	def createLowerMAC(self, config, myFUN):
		return(wifimac.lowerMAC.getFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))

	def createConvergence(self, config, myFUN):
                return(wifimac.convergence.getFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))

	def createManagement(self, config, myFUN):
		return(wifimac.management.getFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))
