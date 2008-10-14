import os
import CNBuildSupport
from CNBuildSupport import CNBSEnvironment
import wnsbase.RCS as RCS

commonEnv = CNBSEnvironment(PROJNAME       = 'wifimac',
                            AUTODEPS       = ['wns', 'dll'],
                            PROJMODULES    = ['BASE', 'CONVERGENCE', 'LOWERMAC', 'DRAFTN', 'HELPER', 'MANAGEMENT', 'PATHSELECTION', 'TEST'],
                            LIBRARY        = True,
                            SHORTCUTS      = True,
                            FLATINCLUDES   = False,
			    REVISIONCONTROL = RCS.Bazaar('../', 'WiFiMAC', 'main', '0.2'), 
                            )
Return('commonEnv')
