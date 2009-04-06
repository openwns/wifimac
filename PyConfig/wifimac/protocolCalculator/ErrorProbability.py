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

import math

def erfc( x,
          p  =  0.3275911,
          a1 =  0.254829592,
          a2 = -0.284496736,
          a3 =  1.421413741,
          a4 = -1.453152027,
          a5 =  1.061405429):
    t = 1.0 / (1.0 + p*x)
    return  ( (a1 + (a2 + (a3 + (a4 + a5*t)*t)*t)*t)*t ) * math.exp(-x*x)

class ErrorProbability:
    guardInterval = None

    def __init__(self, guardInterval):
        self.guardInterval = guardInterval

    def getError(self, postSNR_dB, packetLength_bit, modulation, coding,
                 sqrt = math.sqrt, pow = math.pow):

        SNR = 10**(float(postSNR_dB)/10)

        if(self.guardInterval_us == 0.8e-6):
            SNR *= 0.8
        elif(self.guardInterval_us == 0.4e-6):
            SNR *= 0.9
        else:
            assert(False)

        # Symbol error probability
        p_ser = -1.0;
        # Raw bit error probability
        p_ber = -1.0;

        if(modulation == "BPSK"):
            p_ser = self.Q(sqrt(2.0*SNR))
            p_ber = p_ser
        elif(modulation == "QPSK"):
            p_ser = 2.0 * self.Q(sqrt(SNR)) * (1.0 - 0.5 * self.Q(sqrt(SNR)))
            p_ber = 0.5 * p_ser
        elif(modulation == "16QAM"):
            p_sqrt16 = 3.0/2.0 * self.Q(sqrt((3.0/15.0) * SNR))
            p_ser = 1 - pow((1.0 - p_sqrt16), 2.0)
            p_ber = p_ser / 4.0
        elif(modulation == "64QAM"):
            p_sqrt64 = 7.0/4.0 * self.Q(sqrt((3.0/63.0) * SNR))
            p_ser = 1 - pow((1.0 - p_sqrt64), 2.0);
            p_ber = p_ser / 6.0;
        else:
            assert(False)
        assert(p_ser >= 0.0 and p_ser <= 1.0)
        assert(p_ber >= 0.0 and p_ber <= 1.0)

        # First error event probability
        p_u = -1.0
        if(coding == "1/2"):
            p_u = min(1.0, self.pu12(p_ber))
        elif(coding == "2/3"):
            p_u = min(1.0, self.pu23(p_ber))
        elif(coding == "3/4"):
            p_u = min(1.0, self.pu34(p_ber))
        elif(coding == "5/6"):
            p_u = min(1.0, self.pu56(p_ber))
        else:
            assert(False)
        assert(p_u >= 0.0 and p_u <= 1.0)

        # Packet error probability
        p_per = 1.0 - pow(1.0 - p_u, packetLength_bit)
        assert(p_per >= 0.0 and p_ber <= 1.0)

        return (p_per, p_ber, p_u, p_ser)

    def getPER(self, postSNR_dB, packetLength_bit, modulation, coding):
        (p_per, p_ber, p_u, p_ser) = self.getError(postSNR_dB, packetLength_bit, modulation, coding)
        return(p_per)

    def pd(self, p, d, pow=math.pow):
        """ Chernoff - Approximation """
        return(pow(4.0*p*(1.0-p), d/2.0))

    def pu12(self, rawBer):
        return(11 * self.pd(rawBer,10) +
               38 * self.pd(rawBer,12) +
               193 * self.pd(rawBer, 14) +
               1331 * self.pd(rawBer, 16) +
               7275 * self.pd(rawBer, 18) +
               40406 * self.pd(rawBer, 20) +
               234969 * self.pd(rawBer, 22) )
    def pu23(self, rawBer):
        return( self.pd(rawBer,6) +
                16 * self.pd(rawBer, 7) +
                48 * self.pd(rawBer, 8) +
                158 * self.pd(rawBer, 9) +
                642 * self.pd(rawBer, 10) +
                2435 * self.pd(rawBer, 11) +
                6174 * self.pd(rawBer, 12) +
                34705 * self.pd(rawBer, 13) +
                131585 * self.pd(rawBer, 14) +
                499608 * self.pd(rawBer, 15))
    def pu34(self, rawBer):
        return( 8 * self.pd(rawBer, 5) +
                31 * self.pd(rawBer, 6) +
                160 * self.pd(rawBer, 7) +
                892 * self.pd(rawBer, 8) +
                4512 * self.pd(rawBer, 9) +
                23307 * self.pd(rawBer, 10) +
                121077 * self.pd(rawBer, 11) +
                625059 * self.pd(rawBer, 12) +
                3234886 * self.pd(rawBer, 13) +
                16753077 * self.pd(rawBer, 14))
    def pu56(self, rawBer):
        return( 14 * self.pd(rawBer, 4) +
                69 * self.pd(rawBer, 5) +
                654 * self.pd(rawBer, 6) +
                4996 * self.pd(rawBer, 7) +
                39677 * self.pd(rawBer, 8) +
                314973 * self.pd(rawBer, 9) +
                2503576 * self.pd(rawBer, 10) +
                19875546 * self.pd(rawBer, 11))

    def Q(self, x):
        return(erfc(x/1.4142135623730951) / 2.0)

