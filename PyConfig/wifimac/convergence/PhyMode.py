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
from wns import dBm, dB, fromdB
from wns.PyConfig import Sealed

import wifimac.Logger

class PhyMode(Sealed):
	dataBitsPerSymbol = None
	modulation = None
	codingRate = None
	logger = None
	constFrameSizeBits = None
	minRSS = None
	minSINR = None

	def __init__(self, dataBitsPerSymbol, modulation, codingRate, parentLogger, constFrameSizeBits = 0, minRSS = dBm(0), minSINR = dB(0)):
		self.dataBitsPerSymbol = dataBitsPerSymbol
		self.modulation = modulation
		self.codingRate = codingRate
		self.constFrameSizeBits = constFrameSizeBits
		self.minRSS = minRSS
		self.minSINR = minSINR
		self.logger = wifimac.Logger.Logger(modulation+codingRate, parentLogger) 

class PhyModes(Sealed):
	numPhyModes = None
	logger = None
	
	def __init__(self, logger):
		self.numPhyModes = 8
		self.logger = logger
	
	def getNumPhyModes():
		return self.numPhyModes
	
	def getPhyModePreamble(self):
		return (PhyMode(dataBitsPerSymbol = 24,
				modulation = "BPSK",
				codingRate = "1/2",
				constFrameSizeBits = 4*24,
				minSINR = dB(6.0),
                                parentLogger = self.logger))
	def getPhyMode0(self):
		return (PhyMode(dataBitsPerSymbol = 24,
				modulation = "BPSK",
				codingRate = "1/2",
				minSINR = dB(6.0),
                                parentLogger = self.logger))
	
	
	def getPhyMode1(self):
		return(PhyMode(dataBitsPerSymbol = 36,
			       modulation = "BPSK",
			       codingRate = "3/4",
			       minSINR = dB(8.8),
			       parentLogger = self.logger))
		
	def getPhyMode2(self):
		return(PhyMode(dataBitsPerSymbol = 48,
			       modulation = "QPSK",
			       codingRate = "1/2",
			       minSINR = dB(8.8),
			       parentLogger = self.logger))
		
	def getPhyMode3(self):
		return(PhyMode(dataBitsPerSymbol = 72,
			       modulation = "QPSK",
			       codingRate = "3/4",
			       minSINR = dB(12.0),
			       parentLogger = self.logger))
		
	def getPhyMode4(self):
		return(PhyMode(dataBitsPerSymbol = 96,
			       modulation = "16QAM",
			       codingRate = "1/2",
			       minSINR = dB(15.4),
			       parentLogger = self.logger))
		
	def getPhyMode5(self):
		return(PhyMode(dataBitsPerSymbol = 144,
			       modulation = "16QAM",
			       codingRate = "3/4",
			       minSINR = dB(18.8),
			       parentLogger = self.logger))
		
	def getPhyMode6(self):
		return(PhyMode(dataBitsPerSymbol = 192,
			       modulation = "64QAM",
			       codingRate = "2/3",
			       minSINR = dB(23.5),
			       parentLogger = self.logger))
		
	def getPhyMode7(self):
		return(PhyMode(dataBitsPerSymbol = 216,
			       modulation = "64QAM",
			       codingRate = "3/4",
			       minSINR = dB(24.8),
			       parentLogger = self.logger))
