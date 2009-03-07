import wifimac.convergence
import wifimac.draftn
import wifimac.lowerMAC
import wifimac.pathselection
import wifimac.management
import wifimac.protocolCalculator

from openwns.pyconfig import Sealed

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
		self.names['sinrMIB'] = 'sinrMIB'
		self.names['perMIB'] = 'perMIB'
                self.names.update(wifimac.convergence.names)
		self.names.update(wifimac.pathselection.names)
		self.names.update(wifimac.management.names)
		self.names.update(wifimac.lowerMAC.names)
		self.names.update(wifimac.draftn.names)
		self.names.update(wifimac.protocolCalculator.names)

	def createLowerMAC(self, config, myFUN):
		return(wifimac.lowerMAC.getFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))

	def createConvergence(self, config, myFUN):
                return(wifimac.convergence.getFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))

	def createManagement(self, config, myFUN):
		return(wifimac.management.getFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))

	def createManagementServices(self, config):
		myServices = []
		myServices.append(wifimac.management.InformationBases.SINR(serviceName = self.names['sinrMIB'] + str(self.transceiverAddress),
									   parentLogger = self.logger))
		myServices.append(wifimac.management.InformationBases.PER(serviceName = self.names['perMIB'] + str(self.transceiverAddress),
									  config = config.perMIB,
									  parentLogger = self.logger))
		myServices.append(wifimac.protocolCalculator.ProtocolCalculator(serviceName = self.names['protocolCalculator'] + str(self.transceiverAddress),
										parentLogger = self.logger))
		return(myServices)

class DraftN(Basic):
	def createLowerMAC(self, config, myFUN):
		return(wifimac.draftn.getLowerMACFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))
	def createConvergence(self, config, myFUN):
                return(wifimac.draftn.getConvergenceFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))


