import wx
import os

class PrefFrame(wx.MiniFrame):
	def __init__(self, *args, **kwds):
		wx.MiniFrame.__init__(self, *args, **kwds)
		self.parent = self.GetParent()
		mainsizer = wx.BoxSizer(wx.VERTICAL)
		portsizer = wx.FlexGridSizer(2,2)
		self.gpsout_port_ctrl = wx.ComboBox(self,-1,'',size=(175,30))
		self.radio_port_ctrl = wx.ComboBox(self,-1,'',size=(175,30))
		portsizer.Add(wx.StaticText(self,-1,"Radio Port"),0)
		portsizer.Add(self.radio_port_ctrl,1)
		portsizer.Add(wx.StaticText(self,-1,"GPS Loopback"),0)
		portsizer.Add(self.gpsout_port_ctrl,1)
		ports = self.GetSerialPorts()
		for port in ports:
			self.gpsout_port_ctrl.Append(port)
			self.radio_port_ctrl.Append(port)
		if len(ports) > 0:
			self.radio_port_ctrl.SetValue(ports[0])
		if len(ports) > 1:
			self.gpsout_port_ctrl.SetValue(ports[1])
		save = wx.Button(self,-1,"Apply")
		save.SetDefault()
		mainsizer.Add(portsizer,0,wx.LEFT|wx.TOP|wx.RIGHT,3)
		mainsizer.Add(save,0,wx.ALIGN_CENTER_HORIZONTAL|wx.BOTTOM|wx.TOP,5)
		self.SetSizer(mainsizer)
		wx.EVT_BUTTON(self,save.GetId(),self.OnSave)
		wx.EVT_CLOSE(self,self.OnSave)

	def OnSave(self,evt):
		radio_port = self.radio_port_ctrl.GetValue()
		if radio_port != "":
			self.parent.radio.radio = self.parent.radio.GetRadio(radio_port,9600)
	 	gpsout_port = self.gpsout_port_ctrl.GetValue()
	 	if gpsout_port != "":
			self.parent.radio.gpsout = self.parent.radio.GetRadio(gpsout_port,4800)
	 	
	def GetSerialPorts(self):
		if "__WXMAC__" in wx.PlatformInfo:
			d = os.listdir('/dev')
			l = []
			for i in d:
				if (('serial' in i) or ('Serial' in i)) and ('tty' in i):
					l.append('/dev/'+i)
			return l
		else:
			pass
			#figure out what to do on other platforms
			
    #TODO
        #get joystick axes programatically