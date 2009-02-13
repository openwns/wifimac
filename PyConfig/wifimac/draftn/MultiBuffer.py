from wns.Sealed import Sealed
import wns.FUN
import wns.PyConfig

import wifimac.Logger


class MultiBuffer(wns.FUN.FunctionalUnit):

    __plugin__ = 'wifimac.lowerMAC.MultiBuffer'
    """ Name in FU Factory """

    size = None
    sizeUnit = None
    sendSize = 10
    queueSelector = None

    def __init__(self, functionalUnitName, commandName, parentLogger, size, sizeUnit, selector, **kw):
        super(MultiBuffer, self).__init__(functionalUnitName = functionalUnitName, commandName = commandName)
        self.logger = wifimac.Logger.Logger(name = "MultiBuffer", parent = parentLogger)
        wns.PyConfig.attrsetter(self, kw)
	self.size = size
	self.sizeUnit = sizeUnit
	self.queueSelector = selector
