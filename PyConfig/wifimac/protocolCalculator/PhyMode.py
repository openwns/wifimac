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

class PhyMode:

    def getDBPS(self, modulation, coding):
        CBPS = 0
        if(modulation == "BPSK"):
            CBPS = 52
        elif(modulation == "QPSK"):
            CBPS = 104
        elif(modulation == "16QAM"):
            CBPS = 208
        elif(modulation == "64QAM"):
            CBPS = 312
        else:
            assert(False)

        if(coding == "1/2"):
            return(CBPS/2)
        elif(coding == "2/3"):
            return(CBPS*2/3)
        elif(coding == "3/4"):
            return(CBPS*3/4)
        elif(coding == "5/6"):
            return(CBPS*5/6)
        else:
            assert(False)
