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

class MappingObject(Sealed):
	sinr = None
	ber = None
	def __init__(self, _sinr, _ber):
		self.sinr = _sinr
		self.ber = _ber

class sinr2berMapping(Sealed):
	mapping = None

	def __init__(self, rawMapping):
		self.mapping = []
		for ii in rawMapping:
			self.mapping.append(MappingObject(ii[0], ii[1]))

class PhyMode(Sealed):
	dataBitsPerSymbol = None
	modulation = None
	codingRate = None
	sinr2ber = None
	logger = None
	constFrameSizeBits = None
	minRSS = None
	minSINR = None

	def __init__(self, dataBitsPerSymbol, modulation, codingRate, mapping, parentLogger, constFrameSizeBits = 0, minRSS = dBm(0), minSINR = dB(0)):
		self.dataBitsPerSymbol = dataBitsPerSymbol
		self.modulation = modulation
		self.codingRate = codingRate
		self.sinr2ber = sinr2berMapping(mapping)
		self.constFrameSizeBits = constFrameSizeBits
		self.minRSS = minRSS
		self.minSINR = minSINR
		self.logger = wifimac.Logger.Logger(modulation+codingRate, parentLogger) 

class PhyModesWARP2(Sealed):
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
                                       mapping = [ [2,1],
                                                   [3,0.00107150136714],
                                                   [4,0.00004853425327],
                                                   [5,0.00000164755042],
                                                   [6,0.00000004083685],
                                                   [7,0]
                                                 ],
				minSINR = dB(6),
                                parentLogger = self.logger))
	def getPhyMode0(self):
		return (PhyMode(dataBitsPerSymbol = 24,
                                       modulation = "BPSK",
                                       codingRate = "1/2",
                                       mapping = [ [2,1],
                                                   [3,0.00107150136714],
                                                   [4,0.00004853425327],
                                                   [5,0.00000164755042],
                                                   [6,0.00000004083685],
                                                   [7,0]
                                                 ],
				minSINR = dB(6),
                                parentLogger = self.logger))

	def getPhyMode1(self):
		return(PhyMode(dataBitsPerSymbol = 36,
                                modulation = "BPSK",
                                codingRate = "3/4",
                                mapping = [ [5,1],
                                            [6,0.00175691042724],
                                            [7,0.00004703157636],
                                            [8,0.00000112703005],
                                            [9,0.00000000694479],
                                            [10,0]
                                          ],
			       minSINR = dB(9),
			       parentLogger = self.logger))
		
	def getPhyMode2(self):
		return(PhyMode(dataBitsPerSymbol = 48,
                                modulation = "QPSK",
                                codingRate = "1/2",
                                mapping = [ [5,1],
                                            [6,0.00097375216481],
                                            [7,0.00004694665308],
                                            [8,0.00000165156522],
                                            [9,0.00000003923983],
                                            [10,0]
                                          ],
			       minSINR = dB(9),
			       parentLogger = self.logger))
		
	def getPhyMode3(self):
		return(PhyMode(dataBitsPerSymbol = 72,
                                modulation = "QPSK",
                                codingRate = "3/4",
                                mapping = [ [8,1],
                                            [9,0.00178691843879],
                                            [10,0.00004858613127],
                                            [11,0.00000118976224],
                                            [12,0.00000001736198],
                                            [13,0]
                                          ],
			       minSINR = dB(12),
			       parentLogger = self.logger))
		
	def getPhyMode4(self):
		return(PhyMode(dataBitsPerSymbol = 96,
                                modulation = "16QAM",
                                codingRate = "1/2",
                                mapping = [ [11,1],
                                            [12,0.00179430832295],
                                            [13,0.00014361553146],
                                            [14,0.00000857841424],
                                            [15,0.00000033003916],
                                            [16,0]
                                          ],
			       minSINR = dB(15),
			       parentLogger = self.logger))
		
	def getPhyMode5(self):
		return(PhyMode(dataBitsPerSymbol = 144,
                                modulation = "16QAM",
                                codingRate = "3/4",
                                mapping = [ [15,1],
                                            [16,0.00049386779369],
                                            [17,0.00001747004646],
                                            [18,0.00000046144492],
                                            [19,0.00000000531213],
                                            [20,0]
                                          ],
			       minSINR = dB(19),
			       parentLogger = self.logger))
		
	def getPhyMode6(self):
		return(PhyMode(dataBitsPerSymbol = 192,
                                modulation = "64QAM",
                                codingRate = "2/3",
                                mapping = [ [16,1],
                                            [17,0.91701206300619],
                                            [18,0.08384105954223],
                                            [19,0.00005588733500],
                                            [20,0.00000415194108],
                                            [21,0.00000019989699],
                                            [22,0.00000000427041],
                                            [23,0]
                                          ],
			       minSINR = dB(22),
			       parentLogger = self.logger))
		
	def getPhyMode7(self):
		return(PhyMode(dataBitsPerSymbol = 216,
                                modulation = "64QAM",
                                codingRate = "3/4",
                                mapping = [ [19,1],
                                            [20,0.00245152293285],
                                            [21,0.00013780810443],
                                            [22,0.00000672582611],
                                            [23,0.00000025007500],
                                            [24,0]
                                          ],
			       minSINR = dB(23),
			       parentLogger = self.logger))
		
class PhyModesImproved(Sealed):
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
				mapping = [ [-4,1],
					    [-3,0.00107150136714],
					    [-2,0.00004853425327],
					    [-1,0.00000164755042],
					    [0,0.00000004083685],
					    [1,0]
					    ],
				minSINR = dB(0),
                                parentLogger = self.logger))
	def getPhyMode0(self):
		return (PhyMode(dataBitsPerSymbol = 24,
				modulation = "BPSK",
				codingRate = "1/2",
				mapping = [ [-4,1],
					    [-3,0.00107150136714],
					    [-2,0.00004853425327],
					    [-1,0.00000164755042],
					    [0,0.00000004083685],
					    [1,0]
					    ],
				minSINR = dB(0),
                                parentLogger = self.logger))
	
	
	def getPhyMode1(self):
		return(PhyMode(dataBitsPerSymbol = 36,
			       modulation = "BPSK",
			       codingRate = "3/4",
			       mapping = [ [-1,1],
					   [0,0.00175691042724],
					   [1,0.00004703157636],
					   [2,0.00000112703005],
					   [3,0.00000000694479],
					   [4,0]
					   ],
			       minSINR = dB(3),
			       parentLogger = self.logger))
		
	def getPhyMode2(self):
		return(PhyMode(dataBitsPerSymbol = 48,
			       modulation = "QPSK",
			       codingRate = "1/2",
			       mapping = [ [-1,1],
					   [0,0.00097375216481],
					   [1,0.00004694665308],
					   [2,0.00000165156522],
					   [3,0.00000003923983],
					   [4,0]
					   ],
			       minSINR = dB(3),
			       parentLogger = self.logger))
		
	def getPhyMode3(self):
		return(PhyMode(dataBitsPerSymbol = 72,
			       modulation = "QPSK",
			       codingRate = "3/4",
			       mapping = [ [2,1],
					   [3,0.00178691843879],
					   [4,0.00004858613127],
					   [5,0.00000118976224],
					   [6,0.00000001736198],
					   [7,0]
					   ],
			       minSINR = dB(6),
			       parentLogger = self.logger))
		
	def getPhyMode4(self):
		return(PhyMode(dataBitsPerSymbol = 96,
			       modulation = "16QAM",
			       codingRate = "1/2",
			       mapping = [ [5,1],
					   [6,0.00179430832295],
					   [7,0.00014361553146],
					   [8,0.00000857841424],
					   [9,0.00000033003916],
					   [10,0]
					   ],
			       minSINR = dB(9),
			       parentLogger = self.logger))
		
	def getPhyMode5(self):
		return(PhyMode(dataBitsPerSymbol = 144,
			       modulation = "16QAM",
			       codingRate = "3/4",
			       mapping = [ [9,1],
					   [10,0.00049386779369],
					   [11,0.00001747004646],
					   [12,0.00000046144492],
					   [13,0.00000000531213],
					   [14,0]
					   ],
			       minSINR = dB(13),
			       parentLogger = self.logger))
		
	def getPhyMode6(self):
		return(PhyMode(dataBitsPerSymbol = 192,
			       modulation = "64QAM",
			       codingRate = "2/3",
			       mapping = [ [10,1],
					   [11,0.91701206300619],
					   [12,0.08384105954223],
					   [13,0.00005588733500],
					   [14,0.00000415194108],
					   [15,0.00000019989699],
					   [16,0.00000000427041],
					   [17,0]
					   ],
			       minSINR = dB(16),
			       parentLogger = self.logger))
		
	def getPhyMode7(self):
		return(PhyMode(dataBitsPerSymbol = 216,
			       modulation = "64QAM",
			       codingRate = "3/4",
			       mapping = [ [13,1],
					   [14,0.00245152293285],
					   [15,0.00013780810443],
					   [16,0.00000672582611],
					   [17,0.00000025007500],
					   [18,0]
					   ],
			       minSINR = dB(17),
			       parentLogger = self.logger))
