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

import unittest

class unitTestRelError(unittest.TestCase):
    def getRel(self, val, ref, maxErr):
        if(abs(val) < maxErr):
            return(abs(ref) < maxErr)
        else:
            return(abs(val-ref)/val < maxErr)

class TestMIMO(unitTestRelError):
    def setUp(self):
        self.m = MIMO()

    def test_getPostSNR_dB(self):
        self.assert_(self.getRel(self.m.getPostSNR_dB(5.0, 3, 4), 3.239087409443187, 10e-9))

class TestErrorProbability(unitTestRelError):
    def setUp(self):
        self.e = ErrorProbability(guardInterval_us=0.8)

    def test_getPER(self):
        (per, ber, u, ser) = self.e.getError(5.0, 1000*8, "BPSK", "1/2")
        self.assert_(self.getRel(ser, 0.012244632177757, 10e-6))
        self.assert_(self.getRel(ber, 0.012244632177757, 10e-6))
        self.assert_(self.getRel(u, 3.576260397684864e-06, 10e-6))
        self.assert_(self.getRel(per, 0.028204739777745, 10e-6))

class TestFrameLength(unitTestRelError):
    def setUp(self):
        self.fl = FrameLength()

    def testFrameLength(self):
        self.assert_(self.fl.getAggMSDU_bit(1000*8, 5) == 5118*8)
        self.assert_(self.fl.getAggMPDU_bit(1000*8, 10) == 83520)

class TestDuration(unitTestRelError):
    def setUp(self):
        self.d = Duration(guardInterval_us = 0.8)
        self.n_dbps = [26, 52, 78, 104, 156, 208, 234, 260]

    def test_n_ofdm_sym(self):
        reference_n_ofdm = [[[309,   155,   103,    78,    52,    39,    35,    31],
                             [149,    75,    50,    38,    25,    19,    17,    15]],
                            [[155,    78,    52,    39,    26,    20,    18,    16],
                             [75 ,   38 ,   25 ,   19 ,   13 ,   10 ,    9 ,    8]],
                            [[103,    52,    35,    26,    18,    13,    12,    11],
                             [50 ,   25 ,   17 ,   13 ,    9 ,    7 ,    6 ,    5]],
                            [[78 ,   39 ,   26 ,   20 ,   13 ,   10 ,    9 ,    8],
                             [38 ,   19 ,   13 ,   10 ,    7 ,    5 ,    5 ,    4]]]
        bw = [20.0, 40.0]
        streams = range(1,5)
        for streams_idx in xrange(len(streams)):
            for bw_idx in xrange(len(bw)):
                for n_dbps_idx in xrange(len(self.n_dbps)):
                    n_ofdm = self.d.n_ofdm_sym(1000*8, self.n_dbps[n_dbps_idx], streams[streams_idx], bw[bw_idx])
                    self.assert_(n_ofdm == reference_n_ofdm[streams_idx][bw_idx][n_dbps_idx])

    def testFrameDurations(self):
        self.assert_(self.d.getACK_us(1, 20, "HT-Mix") == 64)
        self.assert_(self.d.getSIFS_us() == 16)
        self.assert_(self.d.getAIFS_us() == 34)
        self.assert_(self.d.getEIFS_us(1, 20, "HT-Mix", 2) == 114)
        self.assert_(self.d.getRTS_us(1, 20, "HT-Mix") == 68)
        self.assert_(self.d.getCTS_us(1, 20, "HT-Mix") == 64)
        self.assert_(self.d.getPreamble_us("HT-Mix", 1) == 36)
        self.assert_(self.d.getBlockACK_us(1, 20, "HT-Mix") == 96)
        self.assert_(self.d.getPPDU_us(1000*8, "No", 0, self.n_dbps[0], 1, 20, "HT-Mix") == 1320)
        self.assert_(self.d.getPPDU_us(1000*8, "A-MPDU", 10, self.n_dbps[0], 1, 20, "HT-Mix") == 12892)

    def testFrameDurationWithMIMO(self):
        # Bandwidth 20 MHz
        self.assert_(self.d.getPreamble_us("HT-Mix", 3) == 48)
        self.assert_(self.d.getACK_us(3, 20, "HT-Mix") == 60)
        self.assert_(self.d.getEIFS_us(3, 20, "HT-Mix", 2) == 110)
        self.assert_(self.d.getRTS_us(3, 20, "HT-Mix") == 60)
        self.assert_(self.d.getCTS_us(3, 20, "HT-Mix") == 60)
        self.assert_(self.d.getBlockACK_us(3, 20, "HT-Mix") == 68)
        self.assert_(self.d.getPPDU_us(1000*8, "No", 0, self.n_dbps[0], 3, 20, "HT-Mix") == 476)
        self.assert_(self.d.getPPDU_us(1000*8, "A-MPDU", 10, self.n_dbps[0], 3, 20, "HT-Mix") == 4336)

        # Bandwidth 40 MHz
        self.assert_(self.d.getACK_us(3, 40, "HT-Mix") == 56)
        self.assert_(self.d.getEIFS_us(3, 40, "HT-Mix", 2) == 106)
        self.assert_(self.d.getRTS_us(3, 40, "HT-Mix") == 56)
        self.assert_(self.d.getCTS_us(3, 40, "HT-Mix") == 56)
        self.assert_(self.d.getBlockACK_us(3, 40, "HT-Mix") == 60)
        self.assert_(self.d.getPPDU_us(1000*8, "No", 0, self.n_dbps[0], 3, 40, "HT-Mix") == 256)
        self.assert_(self.d.getPPDU_us(1000*8, "A-MPDU", 10, self.n_dbps[0], 3, 40, "HT-Mix") == 2112)

    def testSlotDurations(self):
        self.assert_(self.d.getIdleSlot_us() == 9)
        self.assert_(self.d.getCollisionSlot_us(1, 20, "HT-Mix", 2) == 182)
        self.assert_(self.d.getErrorSlot_us(1000*8, "A-MPDU", 10, self.n_dbps[0], 1, 20, "HT-Mix", 2) == 13170)
        self.assert_(self.d.getSuccessSlot_us(1000*8, "A-MPDU", 10, self.n_dbps[0], 1, 20, "HT-Mix", 2) == 13202)


class TestThroughput(unitTestRelError):
    def setUp(self):
        self.t = Throughput()
        self.MCS = [["BPSK", "1/2"], ["QPSK", "1/2"], ["QPSK", "3/4"], ["16QAM", "1/2"],
                    ["16QAM", "3/4"],["64QAM", "2/3"],["64QAM", "3/4"],["64QAM", "5/6"]]

    def test_getPER_PPDU(self):
        reference_per =   [[1.000000000000000,   0.029316231375810,                   0,                   0,                   0,                   0,                   0],
                           [1.000000000000000,   1.000000000000000,   0.000006663060406,                   0,                   0,                   0,                   0],
                           [1.000000000000000,   1.000000000000000,   0.625311978756484,   0.000000000063736,                   0,                   0,                   0],
                           [1.000000000000000,   1.000000000000000,   1.000000000000000,   0.006426205731480,                   0,                   0,                   0],
                           [1.000000000000000,   1.000000000000000,   1.000000000000000,   1.000000000000000,   0.000006089301586,                   0,                   0],
                           [1.000000000000000,   1.000000000000000,   1.000000000000000,   1.000000000000000,   0.999999999999913,   0.000002626675230,                   0],
                           [1.000000000000000,   1.000000000000000,   1.000000000000000,   1.000000000000000,   1.000000000000000,   0.000664698497407,                   0],
                           [1.000000000000000,   1.000000000000000,   1.000000000000000,   1.000000000000000,   1.000000000000000,   0.048104565295635,                   0]]
        for m_i in xrange(len(self.MCS)):
            for r_i in xrange(len(reference_per[m_i])):
                per = self.t.getPER_PPDU(1000*8, "No", 0, self.MCS[m_i][0], self.MCS[m_i][1], r_i*5.0)
                self.assert_(self.getRel(per, reference_per[m_i][r_i], 10e-6))

    def test_getThroughput(self):
        reference_thrp = [[0,   4.669288738779935,   4.816375677302830,   4.816375677302830,   4.816375677302830,   4.816375677302830,   4.816375677302830],
                          [0,                   0,   7.835403227734334,   7.835455435847209,   7.835455435847209,   7.835455435847209,   7.835455435847209],
                          [0,                   0,   3.330576497106871,   9.937888198124366,   9.937888198757763,   9.937888198757763,   9.937888198757763],
                          [0,                   0,                   0,  11.331828509037281,  11.412268188302425,  11.412268188302425,  11.412268188302425],
                          [0,                   0,                   0,                   0,  13.490642977381638,  13.490725126475548,  13.490725126475548],
                          [0,                   0,                   0,                   0,   0.000000000000938,  14.787391842140782,  14.787430683918670],
                          [0,                   0,                   0,                   0,                   0,  15.344879869521575,  15.355086372360844],
                          [0,                   0,                   0,                   0,                   0,  14.857131999662085,  15.717092337917485]]
        for m_i in xrange(len(self.MCS)):
            for r_i in xrange(len(reference_thrp[m_i])):
                thrp = self.t.getThroughput(1000*8, "No", 0, self.MCS[m_i][0], self.MCS[m_i][1], r_i*5.0, 1, 1, 20, 1)
                #print "%s %s @ %.3f -> %.10f vs %.10f" % (self.MCS[m_i][0], self.MCS[m_i][1], r_i*5.0, thrp, reference_thrp[m_i][r_i])
                self.assert_(self.getRel(thrp, reference_thrp[m_i][r_i], 10e-4))

    def test_getThroughputMIMO(self):
        reference_thrp = [[0,   0.005471129853588,  14.059753890396415,  14.059753954305799,  14.059753954305799,  14.059753954305799,  14.059753954305799],
                          [0,                   0,  16.952660755674703,  17.204301075268816,  17.204301075268816,  17.204301075268816,  17.204301075268816],
                          [0,                   0,                   0,  18.475677055824619,  18.475750577367204,  18.475750577367204,  18.475750577367204],
                          [0,                   0,                   0,   4.765386375403859,  19.370460027599162,  19.370460048426150,  19.370460048426150],
                          [0,                   0,                   0,                   0,  19.948501615554587,  20.151133501259444,  20.151133501259444],
                          [0,                   0,                   0,                   0,                   0,  20.488533616240630,  20.565552699228792],
                          [0,                   0,                   0,                   0,                   0,  16.245046091083289,  20.779220778913675],
                          [0,                   0,                   0,                   0,                   0,   0.022706311257138,  20.779220059085542]]

        for m_i in xrange(len(self.MCS)):
            for r_i in xrange(len(reference_thrp[m_i])):
                thrp = self.t.getThroughput(1000*8, "No", 0, self.MCS[m_i][0], self.MCS[m_i][1], r_i*5.0, 3, 4, 40, 1)
                #print "%s %s @ %.3f -> %.10f vs %.10f" % (self.MCS[m_i][0], self.MCS[m_i][1], r_i*5.0, thrp, reference_thrp[m_i][r_i])
                self.assert_(self.getRel(thrp, reference_thrp[m_i][r_i], 10e-6))

    def test_getThroughputAMPDU(self):
        reference_thrp = [[0,   5.854104899354330,   6.030908405578590,   6.030908405578590,   6.030908405578590,   6.030908405578590,   6.030908405578590],
                          [0,                   0,  11.700960502437844,  11.701038467163961,  11.701038467163961,  11.701038467163961,  11.701038467163961],
                          [0,                   0,   6.322957922509278,  17.032148178603606,  17.032148179689163,  17.032148179689163,  17.032148179689163],
                          [0,                   0,                   0,  21.927145804546637,  22.068965517241381,  22.068965517241381,  22.068965517241381],
                          [0,                   0,                   0,                   0,  31.335492697169261,  31.335683509596553,  31.335683509596553],
                          [0,                   0,                   0,                   0,                   0,  39.662761460575915,  39.662865642042640],
                          [0,                   0,                   0,                   0,                   0,  43.425759978385344,  43.454644215100487],
                          [0,                   0,                   0,                   0,                   0,  44.874269166967721,  47.142015321154979]]

        for m_i in xrange(len(self.MCS)):
            for r_i in xrange(len(reference_thrp[m_i])):
                thrp = self.t.getThroughput(1000*8, "A-MPDU", 10, self.MCS[m_i][0], self.MCS[m_i][1], r_i*5.0, 1, 1, 20, 1)
                #print "%s %s @ %.3f -> %.15f vs %.15f" % (self.MCS[m_i][0], self.MCS[m_i][1], r_i*5.0, thrp, reference_thrp[m_i][r_i])
                self.assert_(self.getRel(thrp, reference_thrp[m_i][r_i], 10e-6))

if __name__ == '__main__':
    import wnsrc
    import sys
    import os

    sys.path.append(os.path.join(wnsrc.pathToSandbox, 'dbg', 'lib', 'PyConfig'))
    from wifimac.protocolCalculator import *

    unittest.main()
