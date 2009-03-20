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

from math import pow, log10

class MIMO:
    def getPostSNR_dB(self, preSNR_dB, n_streams, n_rxAntennas):
        # This function implements the SNR processing at the receiver for each
        # spatial stream using ZF detection and returns the post-processing SNR,
        # Post_SNR, corresponding to the pre-processing SNR values.

        # The Caculation is based on the following assumptions
        # 1. The MIMO channel model is block fading Guassian matrix representing
        # the flat Rayleigh fading MIMO channel
        # 2. The Zero-Forcing detect is used at the receiver and 
        # the post SNR of kth stream is expressed as [A. Paulraj 2003]
        # Deterministic model based on the Chi-quared distribution of Post-SNR is
        # used in this function
        return(float(preSNR_dB) + 10.0 * log10((n_rxAntennas - n_streams + 1) / float(n_streams)))
