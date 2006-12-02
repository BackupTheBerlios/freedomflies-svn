import wx
from bufferedcanvas import *
import os,time

class MyLog(wx.PyLog):
	"Class to allow real time logging of uplink/download/interface events"
	def __init__(self, textCtrl, name, logTime = True):
		wx.PyLog.__init__(self)
		self.tc = textCtrl
		self.logTime = logTime
		try:
			os.mkdir('logs')
		except OSError:
			#directory already exists
			pass
		filename = 'logs/'+time.strftime("%Y%m%d-%H%M")+name+'.txt'
		self.outfile = open(filename,'w')
	
	def DoLogString(self, message):
		if self.logTime:
			message = time.strftime("%X", time.localtime()) + ": " + message
		self.tc.AppendText(message + '\n')
		self.tc.ShowPosition(self.tc.GetNumberOfLines()) #scroll to bottom on write
		self.outfile.write(message + '\n')
		self.outfile.flush()
		print message
#end of class MyLog

def Log(target,message):
	global error_log,downlink_log,uplink_log
	
	if target == 'e':
		error_log.DoLogString(message)
	if target == 'd':
		downlink_log.DoLogString(message)
		print message
	if target == 'u':
		uplink_log.DoLogString(message)

def SetLogs(err,down,up):
	global error_log,downlink_log,uplink_log
	
	error_log = MyLog(err,"errors")
	downlink_log = MyLog(down,"downlink")
	uplink_log = MyLog(up,"uplink")
	