#!/usr/bin/env pythonw
"""Ground station software for FreedomFlies, an opensource UAV.
Surveil the surveillers!

Written by Josh Levinger, Fall 2005

Licensed under the MIT License
--------------------------------------------------------------
Copyright (c) 2005 MIT Media Lab

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--------------------------------------------------------------
"""

#main imports
import time
import math
import sys,os
import threading

#my imports
import prefs
import joystick as joystickClass #avoid name collision
import compass
import horizon
import log
from bufferedcanvas import *
import datalink

#external library imports
try:
	import wx
	import wx.glcanvas
	from wx.lib.dialogs import * #for about box
except ImportError:
	print "cannot import wx"
try:
	import pygame
except ImportError:
	print "cannot import pygame"
try:
	import serial
except ImportError:
	print "cannot import pyserial"


class AppFrame(wx.Frame):
	def __init__(self,*args,**kwds):
		wx.Frame.__init__(self,*args,**kwds)
		
		#init pygame first, may take awhile
		pygame.init()

		#top sizer elements
		self.airspeed_gauge = wx.Gauge(self, -1, 100, size=(20,250), style=wx.GA_VERTICAL|wx.GA_SMOOTH)
		self.altitude_gauge = wx.Gauge(self, -1, 100, size=(20,250), style=wx.GA_VERTICAL|wx.GA_SMOOTH)
		self.horizon = horizon.MyHorizonIndicator(self,-1,(500,250))
		
		#force initial paint
		e = wx.PaintEvent()
		self.horizon.OnPaint(e)
		self.horizon.SetClientSize((400,400))
		
		
		self.airspeed_value = wx.TextCtrl(self,-1,size=(50,20),style=wx.TE_READONLY)
		self.altitude_value = wx.TextCtrl(self,-1,size=(50,20),style=wx.TE_READONLY)
	
		#bottom sizer elements
		button_width = (80,20)
		self.autopilot_button = wx.ToggleButton(self, -1, "Autopilot",size=button_width)
		self.engine_button = wx.ToggleButton(self, -1, "Engine",size=button_width)
		self.logfile_button = wx.ToggleButton(self, -1, "Log to file",size=button_width)
		self.radio_up_button = wx.ToggleButton(self, -1, "Radio Up",size=button_width)
		self.radio_down_button = wx.ToggleButton(self, -1, "Radio Down",size=button_width)
		self.gps_loopback_button = wx.ToggleButton(self, -1, "GPS Loopback",size=button_width)
		
		self.notebook = wx.Notebook(self, -1, size = (200,40), style=0)
		self.error_log_ctrl = wx.TextCtrl(self.notebook, -1, style = wx.TE_MULTILINE|wx.TE_READONLY)
		self.downlink_log_ctrl = wx.TextCtrl(self.notebook, -1, style = wx.TE_MULTILINE|wx.TE_READONLY)
		self.uplink_log_ctrl = wx.TextCtrl(self.notebook, -1, style = wx.TE_MULTILINE|wx.TE_READONLY)
		
		log.SetLogs(self.error_log_ctrl,
		self.downlink_log_ctrl,
		self.uplink_log_ctrl) #send textctrls to log.py
		
		
		self.lat_deg_ctrl = wx.TextCtrl(self, -1,size=(30,20),style=wx.TE_READONLY|wx.TE_RICH2)
		self.lat_min_ctrl = wx.TextCtrl(self, -1,size=(60,20),style=wx.TE_READONLY|wx.TE_RICH2)
		self.lat_dir_text = wx.StaticText(self, -1, " ")
		self.lon_deg_ctrl = wx.TextCtrl(self, -1,size=(30,20),style=wx.TE_READONLY|wx.TE_RICH2)
		self.lon_min_ctrl = wx.TextCtrl(self, -1,size=(60,20),style=wx.TE_READONLY|wx.TE_RICH2)
		self.lon_dir_text = wx.StaticText(self, -1, " ")
		self.gps_error_ctrl = wx.TextCtrl(self, -1,size=(100,20),style=wx.TE_READONLY|wx.TE_RICH2)
		self.throttle_gauge = wx.Gauge(self, -1, 10,size=(100,20),style=wx.GA_HORIZONTAL|wx.GA_SMOOTH)

		#compass replaced by PFD in horizon indicator
		#self.compass = compass.MyCompass(self,-1,(250,250))

		main_box_sizer = wx.BoxSizer(wx.HORIZONTAL)
		main_grid_sizer = wx.GridSizer(2, 1)
	
		top_sizer = wx.BoxSizer(wx.HORIZONTAL)
		bottom_sizer = wx.FlexGridSizer(1, 3,hgap=5)
		
		airspeed_sizer = wx.BoxSizer(wx.HORIZONTAL)
		altitude_sizer = wx.BoxSizer(wx.HORIZONTAL)
		airspeed_sizer.Add(self.airspeed_value,0, wx.ALIGN_CENTER_VERTICAL|wx.LEFT,3)
		airspeed_sizer.Add(self.airspeed_gauge,0,wx.RIGHT,-2)
		altitude_sizer.Add(self.altitude_gauge,0,wx.LEFT,5)
		altitude_sizer.Add(self.altitude_value,0,wx.ALIGN_CENTER_VERTICAL|wx.RIGHT,0)
		
		top_sizer.Add(airspeed_sizer, 0)
		top_sizer.Add(self.horizon, 0,wx.LEFT)
		top_sizer.Add(altitude_sizer, 0)
		main_grid_sizer.Add(top_sizer,1,wx.LEFT,0)
		
		button_sizer = wx.GridSizer(4, 2, vgap=10,hgap=10)
		button_sizer.Add(self.radio_up_button, 0)
		button_sizer.Add(self.autopilot_button, 0)
		button_sizer.Add(self.radio_down_button, 0)
		button_sizer.Add(self.logfile_button, 0)
		button_sizer.Add(self.gps_loopback_button, 0)
		button_sizer.Add(self.engine_button, 0)
		bottom_sizer.Add(button_sizer,1,wx.LEFT|wx.ALIGN_CENTER_VERTICAL,5)
		wx.EVT_TOGGLEBUTTON(self,self.radio_up_button.GetId(),self.OnRadioUpButton)
		wx.EVT_TOGGLEBUTTON(self,self.radio_down_button.GetId(),self.OnRadioDownButton)
		
		self.notebook.AddPage(self.error_log_ctrl, "Error")
		self.notebook.AddPage(self.downlink_log_ctrl, "Downlink")
		self.notebook.AddPage(self.uplink_log_ctrl, "Uplink")
		bottom_sizer.Add(self.notebook,1)
		
		info_sizer = wx.FlexGridSizer(4,2,vgap=5)
		lat_sizer = wx.BoxSizer(wx.HORIZONTAL)
		info_sizer.Add(wx.StaticText(self, -1, "Latitude"),1)
		lat_sizer.Add(self.lat_deg_ctrl,0,wx.RIGHT,5)
		lat_sizer.Add(self.lat_min_ctrl,0,wx.RIGHT,5)
		lat_sizer.Add(self.lat_dir_text,0)
		info_sizer.Add(lat_sizer,0,wx.ALIGN_LEFT,0)
		lon_sizer = wx.BoxSizer(wx.HORIZONTAL)
		info_sizer.Add(wx.StaticText(self, -1, "Longitude"),1)
		lon_sizer.Add(self.lon_deg_ctrl,0,wx.RIGHT,5)
		lon_sizer.Add(self.lon_min_ctrl,0,wx.RIGHT,5)
		lon_sizer.Add(self.lon_dir_text,0)
		info_sizer.Add(lon_sizer,0,wx.ALIGN_LEFT,0)
		
		info_sizer.Add(wx.StaticText(self, -1, "Accuracy"),1)
		info_sizer.Add(self.gps_error_ctrl)
		info_sizer.Add(wx.StaticText(self, -1, "Throttle"),1)
		info_sizer.Add(self.throttle_gauge)
		bottom_sizer.Add(info_sizer, 0, wx.ALIGN_CENTER_VERTICAL|wx.RIGHT)
		
		#compass_sizer = wx.BoxSizer(wx.HORIZONTAL)
		#compass_sizer.Add(self.compass,0)
		
		main_grid_sizer.Add(bottom_sizer,1,wx.LEFT,0)
		main_box_sizer.Add(main_grid_sizer,1,wx.LEFT,0)
		#main_box_sizer.Add(compass_sizer,0,wx.TOP|wx.RIGHT|wx.LEFT,5)
		self.SetAutoLayout(True)
		self.SetSizer(main_box_sizer)
		self.Layout()
		self.SetMinSize((625,525))
		#end of main window items
		
		#Menubar
		self.MenuBar = wx.MenuBar()
		FileMenu = wx.Menu()
		FileMenu.Append(-1, 'Save log')
		FileMenu.AppendSeparator()
		prefs = FileMenu.Append(-1,'Preferences')		
		quit = FileMenu.Append(-1, "E&xit\tAlt-X", "Exit demo")
		wx.EVT_MENU(self,quit.GetId(), self.OnQuit)
		self.MenuBar.Append(FileMenu, '&File')

		HelpMenu = wx.Menu()
		help = HelpMenu.Append(-1, 'About FreedomFlies')
		wx.EVT_MENU(self, help.GetId(), self.OnAbout)
		wx.EVT_MENU(self, prefs.GetId(),self.OnPrefs)
		if "__WXMAC__" in wx.PlatformInfo: 
			wx.App.SetMacAboutMenuItemId(help.GetId())
			wx.App_SetMacHelpMenuTitleName("&Help")
		self.MenuBar.Append(HelpMenu, "&Help")
		
		SetupMenu = wx.Menu()
		cal = SetupMenu.Append(-1,'Calibrate Joystick')
		wx.EVT_MENU(self,cal.GetId(),self.OnJoystickCalibrate)
		gtest = SetupMenu.Append(-1,"Graphics Test")
		wx.EVT_MENU(self,gtest.GetId(),self.OnGraphicsTest)
		self.MenuBar.Append(SetupMenu,"Setup")
		self.SetMenuBar(self.MenuBar)
		#end of menubar
		
		#global objects
		#self.joystickCalibrated = True
		self.joystickCalibrated = False
		self.joystick = joystickClass.Joystick()	
		self.gpsout_port = ""
		self.radio_port = ""
		#init radio, set for real by prefs dialog
		self.radio = datalink.radiolink(self,self.radio_port,self.gpsout_port)
		
		#startup
		self.OnPrefs(None)
	#end __init__
		
	def OnQuit(self, event):
		self.Close()
		sys.exit()
	def OnAbout(self, event):
		about = ScrolledMessageDialog(self, __doc__, "About...")
		about.ShowModal()
	def OnPrefs(self,event):
		p = prefs.PrefFrame(self,-1,"Preferences",(975,265),(300,150))
		p.Show()
	def OnGraphicsTest(self,event):
		max_pitch = 20
		max_roll = 45
		incr = 1
		sleep_time = 0.01 #sec
	
		for i in range(0,max_pitch,incr):
			self.horizon.SetPitch(i)
			time.sleep(sleep_time)
			wx.Yield()
		for i in range(max_pitch,-max_pitch,-incr):
			self.horizon.SetPitch(i)
			time.sleep(sleep_time)
			wx.Yield()
		for i in range(-max_pitch,0,incr):
			self.horizon.SetPitch(i)
			time.sleep(sleep_time)
			wx.Yield()
			
		self.horizon.SetPitch(0)
		self.horizon.SetRoll(0)
			
		for i in range(0,max_roll,incr):
			self.horizon.SetRoll(i)
			time.sleep(sleep_time)
			wx.Yield()
		for i in range(max_roll,-max_roll,-incr):
			self.horizon.SetRoll(i)
			time.sleep(sleep_time)
			wx.Yield()
		for i in range(-max_roll,0,incr):
			self.horizon.SetRoll(i)	
			time.sleep(sleep_time)
			wx.Yield()
			
		self.horizon.SetPitch(0)
		self.horizon.SetRoll(0)
			
		#for i in range(0,361,2):
		#	self.compass.SetHeading(i)
		#	wx.Yield()
		
	def OnJoystickCalibrate(self,event):
		self.calib = joystickClass.JoyFrame(self,-1,"Joystick Calibration",(975,30),(200,225))
		self.calib.Layout()
		self.calib.Show()
		
	def OnRadioUpButton(self,event):
		btn = event.GetEventObject()
		if btn.GetValue() == True:
			if self.joystickCalibrated is False:
				log.Log('e',"Calibrate Joystick")
				self.OnJoystickCalibrate(None)
			self.radio.StartUplinkThread()
		if btn.GetValue() == False:
			self.radio.StopUplinkThread()

	
	def OnRadioDownButton(self,event):
		btn = event.GetEventObject()
		if btn.GetValue() == True:
			#radio_port, gpsout_port are set by prefs dialog
			black = wx.TextAttr(wx.BLACK)
			self.lat_deg_ctrl.SetDefaultStyle(black)
			self.lat_min_ctrl.SetDefaultStyle(black)
			self.lon_deg_ctrl.SetDefaultStyle(black)
			self.lon_min_ctrl.SetDefaultStyle(black)
			#self.accuracy_ctrl.SetDefaultStyle(black)
			self.radio.StartDownlinkThread()
		if btn.GetValue() == False:	
			self.radio.StopDownlinkThread()
			red = wx.TextAttr(wx.RED)
			self.lat_deg_ctrl.SetDefaultStyle(red)
			self.lat_min_ctrl.SetDefaultStyle(red)
			self.lon_deg_ctrl.SetDefaultStyle(red)
			self.lon_min_ctrl.SetDefaultStyle(red)
			#self.accuracy_ctrl.SetDefaultStyle(red)
			#set old data to red
	
				
	def UpdateLatitude(self,lat_deg,lat_min,lat_dir):
		self.lat_deg_ctrl.SetValue(lat_deg)
		self.lat_min_ctrl.SetValue(lat_min)
		self.lat_dir_text.SetLabel(lat_dir)
	def UpdateLongitude(self,lon_deg,lon_min,lon_dir):	
		self.lon_deg_ctrl.SetValue(lon_deg)
		self.lon_min_ctrl.SetValue(lon_min)
		self.lon_dir_text.SetLabel(lon_dir)
	def UpdateAltitude(self,alt):
		self.altitude_value.SetValue(alt)
		#self.altitude_gauge.SetValue(alt/100)
	def UpdateAirspeed(self,v):
		self.airspeed_value.SetValue(v)
		#self.airspeed_gauge.SetValue(v/100)
# end of class AppFrame

class MyApp(wx.App):
	def OnInit(self):
		#pygame.init()
		wx.InitAllImageHandlers()
		self.win = AppFrame(None, -1, "Freedom Flies",(50,25),(625,525))
		self.win.Layout()
		self.win.Show()
		self.SetTopWindow(self.win)
		self.Bind(wx.EVT_CLOSE,self.OnExit)
		return True
		
	def OnExit(self):
		self.radio.StopUplinkThread()
		self.radio.StopDownlinkThread()
		pygame.quit()
# end of class MyApp

if __name__ == "__main__":
	app = MyApp(0)
	try:
		app.MainLoop()
	finally:
		app.radio.radio.close()
		pygame.quit()