#!/usr/bin/pythonw
# joystick.py

import wx
import pygame
import log

class JoyPanel(wx.Panel):
	def __init__(self, parent, *args, **kwargs):
		wx.Panel.__init__(self, parent, *args, **kwargs)
		self.timer = wx.Timer(self, 1)
		self.Bind(wx.EVT_TIMER, self.OnTimer, self.timer)
		
		#parent's already got the joystick
		self.joystick = parent.joystick
			
		sizer = wx.BoxSizer(wx.VERTICAL)
		self.control = JoyGauge(self,self.joystick)
		self.throttle_pos = wx.Gauge(self, -1,100,size=(100,20),style=wx.GA_HORIZONTAL|wx.GA_SMOOTH)
		#self.text = wx.StaticText(self, -1, "Move joystick to the extreme of each axis.",size=(150,50))
		#self.CalibratedButton = wx.Button(self,-1,"Calibrated")
		sizer.Add(self.control,0,wx.ALIGN_CENTER_HORIZONTAL|wx.ALL,5)
		sizer.Add(self.throttle_pos,0,wx.ALIGN_CENTER_HORIZONTAL)
		#sizer.Add(self.text,1,wx.ALIGN_CENTER)
		#sizer.Add(self.CalibratedButton,1,wx.ALIGN_CENTER)
		#self.Bind(wx.EVT_BUTTON,self.OnCalibrated,self.CalibratedButton)
		#self.CalibratedButton.SetDefault()

		self.SetSizer(sizer)
		#self.SetMinSize((200,300))
		#self.SetAutoLayout(True)
		#self.Layout()
		
		#start calibration automatically
		self.timer.Start(10) #every 10 ms, or 100hz
		x,y = self.joystick.getPos()
		if (x == -99900) or (y == -99900): #absurd values, trap errors
			log.Log('e',"Unable to access joystick")
			self.timer.Stop()
			wx.Bell()
	#end method __init__
	
	def OnTimer(self, event):
		pygame.event.pump()
		x,y = self.joystick.getPos()
		throttle = self.joystick.getThrottle()
		self.control.Update()
		self.throttle_pos.SetValue(throttle)
		
	def OnClose(self, event):
		self.timer.Stop()
		self.Close()
		
	def OnCalibrated(self, event):
		self.parent.joystickCalibrated = True
		e = wx.CloseEvent()
		self.OnClose(e)
		
class Joystick(object):
	def __init__(self,*args,**kwds):
		try:
			self.stick = pygame.joystick.Joystick(0)
			status = self.stick.init() #returns none
			if(status == None):
				 log.Log('e',"joystick successfully initialized")
			else:
				log.Log('e',"could not initialize joystick, status " + str(status))
		except pygame.error,err:
			log.Log('e',str(err))
			self.stick = None

	def getPos(self):
		"ranges 0-100"
		try:
			raw_x = self.stick.get_axis(0)
			raw_y = self.stick.get_axis(1)
		except AttributeError:
			raw_x,raw_y = -999,-999
			#an invalid raw position, used to trap errors
		x = int(raw_x * 100)
		y = int(raw_y * -100)
		return x,y
		
	def getThrottle(self):
		"ranges 0-100"
		try:
			raw_pos = self.stick.get_axis(3)  #csik changed for his SAITEK joystick from 2, which apparently was "z rotation"
			throttle = (raw_pos-1)*-50 #should range (0,100)
		except AttributeError:
			log.Log('e',"could not access throttle")
			throttle = -1
		return throttle
		
class JoyGauge(wx.Panel):
	#shamelessly stolen from the wxPython Demo, but using pygame for joystick
    def __init__(self, parent, stick):

        self.stick = stick
        size = (150,150)
        
        wx.Panel.__init__(self, parent, -1, size=size)

        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda e: None)

        self.buffer = wx.EmptyBitmap(*size)
        dc = wx.BufferedDC(None, self.buffer)
        self.DrawFace(dc)
        self.DrawJoystick(dc)
       
    def OnSize(self, event):
        # The face Bitmap init is done here, to make sure the buffer is always
        # the same size as the Window
        w, h  = self.GetClientSize()
        self.buffer = wx.EmptyBitmap(w,h)
        dc = wx.BufferedDC(wx.ClientDC(self), self.buffer)
        self.DrawFace(dc)
        self.DrawJoystick(dc)


    def DrawFace(self, dc):
        dc.SetBackground(wx.Brush(self.GetBackgroundColour()))
        dc.Clear()


    def OnPaint(self, evt):
        # When dc is destroyed it will blit self.buffer to the window,
        # since no other drawing is needed we'll just return and let it
        # do it's thing
        dc = wx.BufferedPaintDC(self, self.buffer)


    def DrawJoystick(self, dc):
        # draw the gauge as a maxed square in the center of this window.
        w, h = self.GetClientSize()
        edgeSize = min(w, h)

        xorigin = (w - edgeSize) / 2
        yorigin = (h - edgeSize) / 2
        center = edgeSize / 2

        # Restrict our drawing activities to the square defined
        # above.
        dc.SetClippingRegion(xorigin, yorigin, edgeSize, edgeSize)

        # Optimize drawing a bit (for Win)
        dc.BeginDrawing()

        dc.SetBrush(wx.Brush(wx.Colour(251, 252, 237)))
        dc.DrawRectangle(xorigin, yorigin, edgeSize, edgeSize)

        dc.SetPen(wx.Pen(wx.BLACK, 1, wx.DOT_DASH))

        dc.DrawLine(xorigin, yorigin + center, xorigin + edgeSize, yorigin + center)
        dc.DrawLine(xorigin + center, yorigin, xorigin + center, yorigin + edgeSize)

        if self.stick:
            # Get the joystick position as a float
            joyx,joyy = self.stick.getPos()
            
            #Get the joystick range of motion
            xmin = -100 #self.stick.GetXMin()
            xmax = 100 #self.stick.GetXMax()
            if xmin < 0:
                xmax += abs(xmin)
                joyx += abs(xmin)
                xmin = 0
            xrange = max(xmax - xmin, 1)

            ymin = -100 #self.stick.GetYMin()
            ymax = 100 #self.stick.GetYMax()
            if ymin < 0:
                ymax += abs(ymin)
                joyy += abs(ymin)
                ymin = 0
            yrange = max(ymax - ymin, 1)

            # calc a ratio of our range versus the joystick range
            xratio = float(edgeSize) / xrange
            yratio = float(edgeSize) / yrange

            # calc the displayable value based on position times ratio
            xval = int(joyx * xratio)
            yval = int(joyy * yratio)

            # and normalize the value from our brush's origin
            x = xval + xorigin
            y = yval + yorigin

            # Now to draw it.
            dc.SetPen(wx.Pen(wx.RED, 2))
            dc.CrossHair(x, y)

        # Turn off drawing optimization
        dc.EndDrawing()

    def Update(self):
        dc = wx.BufferedDC(wx.ClientDC(self), self.buffer)
        self.DrawFace(dc)
        self.DrawJoystick(dc)