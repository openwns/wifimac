srcFiles = dict()

srcFiles['BASE'] = [
    'src/WiFiMAC.cpp',
    'src/MultiRadioLayer2.cpp',

    'src/convergence/PhyUser.cpp',
    'src/convergence/OFDMAAccessFunc.cpp',
    'src/convergence/ErrorModelling.cpp',
    'src/convergence/PhyMode.cpp',
    'src/convergence/PhyModeProvider.cpp',
    'src/convergence/FrameSynchronization.cpp',

    'src/scheduler/AcknowledgedTxScheduler.cpp',
    'src/scheduler/BroadcastScheduler.cpp',
    'src/scheduler/Backoff.cpp',
    'src/scheduler/ChannelState.cpp',
    'src/scheduler/StopAndWaitARQ.cpp',

    'src/helper/Keys.cpp',
    'src/helper/HopContextWindowProbe.cpp',

    'src/management/LowerMACManager.cpp',
    'src/management/Beacon.cpp',

    'src/pathselection/VirtualPathSelection.cpp',
    'src/pathselection/MeshForwarding.cpp',
    'src/pathselection/StationForwarding.cpp',
    'src/pathselection/PathSelection.cpp',
    'src/pathselection/BeaconLinkQualityMeasurement.cpp',
    ]

srcFiles['TEST'] = [
    'src/scheduler/tests/BackoffTest.cpp',
    ]

Return('srcFiles')
