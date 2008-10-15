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
srcFiles = dict()

srcFiles['BASE'] = [
    'src/WiFiMAC.cpp',
    'src/Layer2.cpp',
]

srcFiles['CONVERGENCE'] = [
    'src/convergence/PhyUser.cpp',
    'src/convergence/OFDMAAccessFunc.cpp',
    'src/convergence/ErrorModelling.cpp',
    'src/convergence/PhyMode.cpp',
    'src/convergence/PhyModeProvider.cpp',
    'src/convergence/FrameSynchronization.cpp',
    'src/convergence/PreambleGenerator.cpp',
    'src/convergence/TxDurationSetter.cpp',
    'src/convergence/ChannelState.cpp',
]

srcFiles['LOWERMAC'] = [
    'src/lowerMAC/Manager.cpp',
    'src/lowerMAC/RTSCTS.cpp',
    'src/lowerMAC/StopAndWaitARQ.cpp',
    'src/lowerMAC/RateAdaptation.cpp',
    'src/lowerMAC/TXOP.cpp',
    'src/lowerMAC/NextFrameGetter.cpp',

    'src/lowerMAC/timing/DCF.cpp',
    'src/lowerMAC/timing/ConstantWait.cpp',
    'src/lowerMAC/timing/Backoff.cpp',

    'src/lowerMAC/rateAdaptationStrategies/ConstantLow.cpp',
    'src/lowerMAC/rateAdaptationStrategies/SINR.cpp',
]

srcFiles['HELPER'] = [
    'src/helper/Keys.cpp',
    'src/helper/HopContextWindowProbe.cpp',
    'src/helper/FilterFrameType.cpp',
    'src/helper/FilterSize.cpp',
]

srcFiles['MANAGEMENT'] = [
    'src/management/Beacon.cpp',
    'src/management/SINRInformationBase.cpp',
    'src/management/PERInformationBase.cpp',
    'src/management/VirtualCapabilityInformationBase.cpp',
]

srcFiles['PATHSELECTION'] = [
    'src/pathselection/VirtualPathSelection.cpp',
    'src/pathselection/MeshForwarding.cpp',
    'src/pathselection/StationForwarding.cpp',
    'src/pathselection/PathSelectionOverVPS.cpp',
    'src/pathselection/BeaconLinkQualityMeasurement.cpp',
]

srcFiles['TEST'] = [
    'src/lowerMAC/timing/tests/BackoffTest.cpp',
]

Return('srcFiles')
