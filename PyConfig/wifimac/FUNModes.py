import wifimac.convergence
import wifimac.draftn
import wifimac.lowerMAC
import wifimac.pathselection
import wifimac.management

from wns.PyConfig import Sealed


class Basic(Sealed):
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

class DraftN(Basic):
	def createLowerMAC(self, config, myFUN):
		return(wifimac.draftn.getLowerMACFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))
	def createConvergence(self, config, myFUN):
                return(wifimac.draftn.getConvergenceFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))


