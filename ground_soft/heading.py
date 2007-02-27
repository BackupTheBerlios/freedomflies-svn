import pygame
import wx
from wx.glcanvas import *
from OpenGL.GL import *
import circleEvaluator

# OpenGL display code stolen from OpenGC
# http://opengc.sourceforge.net/
# Copyright (c) 2001-2004 Damion Shelton
# modified to Python by Josh Levinger

class MyHeadingIndicator(GLCanvas):
	def __init__(self,parent,id,sizeT):
		GLCanvas.__init__(self,parent,id,size=sizeT)
		#default size is (500,250)
		self.init = False
		self.Bind(wx.EVT_ERASE_BACKGROUND, self.OnEraseBackground)
		self.Bind(wx.EVT_SIZE, self.OnSize)
		self.Bind(wx.EVT_PAINT, self.OnPaint)
		self.heading = 0
		self.refresh = wx.Timer(self,id=1)
		self.refresh.Start(33) #in ms, so 30hz
		self.Bind(wx.EVT_TIMER,self.OnDraw,self.refresh)
	
	def OnEraseBackground(self, event):
		pass # Do nothing, to avoid flashing on MSW.
		
	def OnSize(self, event):
		size = self.GetClientSize()
		if self.GetContext():
			self.SetCurrent()
			glViewport(0, 0, size.width, size.height)
		event.Skip()
	
	def OnPaint(self, event):
		dc = wx.PaintDC(self)
		self.SetCurrent()
		if not self.init:
			self.InitGL()
			self.init = True
		e = wx.PaintEvent()
		self.OnDraw(e)
	
	def InitGL(self):
		m_UnitsPerPixel = 1
		
		m_PhysicalPosition_x = 0
		m_PhysicalPosition_y = 0
		parentPhysicalPosition_x = 0
		parentPhysicalPosition_y = 0
		m_PhysicalSize_x = 130
		m_PhysicalSize_y = 45
		m_Scale_x = 1.0
		m_Scale_y = 1.0
		
		#The location in pixels is calculated based on the size of the
		#gauge component and the offset of the parent guage
		m_PixelPosition_x = GLsizei(int((m_PhysicalPosition_x * m_Scale_x + parentPhysicalPosition_x ) / m_UnitsPerPixel))
		m_PixelPosition_y = GLsizei(int((m_PhysicalPosition_y * m_Scale_y + parentPhysicalPosition_y ) / m_UnitsPerPixel))
		
		#The size in pixels of the gauge is the physical size / mm per pixel
		m_PixelSize_x =  GLint(int(( m_PhysicalSize_x / m_UnitsPerPixel * m_Scale_x)))
		m_PixelSize_y =  GLint(int(( m_PhysicalSize_y / m_UnitsPerPixel * m_Scale_y)))
		
		self.SetCurrent()
		glViewport(m_PixelPosition_x, m_PixelPosition_y, m_PixelSize_x, m_PixelSize_y)
		glMatrixMode(GL_PROJECTION)
		glLoadIdentity()
		
		#Define the projection so that we're drawing in "real" space
		glOrtho(0, m_Scale_x * m_PhysicalSize_x, 0, m_Scale_y * m_PhysicalSize_y, -1, 1)
		#Prepare the modelview matrix
		glMatrixMode(GL_MODELVIEW)
		glLoadIdentity()
		glScalef(m_Scale_x, m_Scale_y, 1.0)

	def GetHeading(self):
		return self.heading
		
	def SetHeading(self,h):
		self.heading = h

	def OnDraw(self,event):
		centerX = 60
		centerY = -35
		radius = 70.0
		indicatorDegreesPerTrueDegrees = 1.5
		
		bigFontSize = 5.0
		littleFontSize = 3.5
		
		buffer = ""
		
		glMatrixMode(GL_MODELVIEW)
		glPushMatrix()
		
		glTranslated(centerX, centerY, 0)
		
		# Draw in gray
		glColor3ub(51,51,76)
		glLineWidth( 1.5 )
		
		# Set up the circle
		aCircle = circleEvaluator.CircleEvaluator()
		aCircle.SetRadius(radius)
		aCircle.SetArcStartEnd(300,60)
		aCircle.SetDegreesPerPoint(5)
		aCircle.SetOrigin(0.0, 0.0)
		
		#Draw the circle
		glBegin(GL_TRIANGLE_FAN)
		glVertex2f(0,0)
		aCircle.Evaluate()
		glEnd()
		
		#Draw the center detent
		glColor3ub(255,255,255)
		glBegin(GL_LINE_LOOP)
		glVertex2f(0.0,radius)
		glVertex2f(-2.8,radius+3.25)
		glVertex2f(2.8,radius+3.25)
		glEnd()
		
		heading = self.GetHeading()
		
		#m_pFontManager->SetSize(m_Font, 5.0, 5.0 );
		#m_pFontManager->Print( -25, 38, &buffer[0], m_Font ); 
	
		#Figure out the nearest heading that's a multiple of 10
		nearestTen = int(heading) - int(heading) % 10
	
		#Derotate by this offset
		glRotated( -1.0*(heading - nearestTen)*indicatorDegreesPerTrueDegrees,0,0,-1)
	
		#Now derotate by 40 "virtual" degrees
		glRotated(-40*indicatorDegreesPerTrueDegrees,0,0,-1)
	
		#Now draw lines 5 virtual degrees apart around the perimeter of the circle
		for i in range(nearestTen-40,nearestTen+40,5):
			#The length of the tickmarks on the compass rose
			tickLength = 0
	
			#Make sure the display heading is between 0 and 360
			displayHeading = (i+720)%360
	
			#If the heading is a multiple of ten, it gets a long tick
			if(displayHeading%10==0):
				tickLength = 4;
	
#				#The x-position of the font (depends on the number of characters in the heading)
#				double fontx;
#			  
#				#Convert the display heading to a string
#				#_itoa( displayHeading/10, buffer, 10);
#				sprintf( buffer, "%d", displayHeading/10 );
#				if(displayHeading%30==0):
#					if(displayHeading>=100):
#						fontx = -bigFontSize
#					else:
#						fontx = -bigFontSize/2
#						
#					m_pFontManager->SetSize(m_Font, bigFontSize, bigFontSize)
#					m_pFontManager->Print(fontx, radius-tickLength-bigFontSize, &buffer[0], m_Font )
#				else:
#					if(displayHeading>=100):
#						fontx = -littleFontSize
#					else:
#						fontx = -littleFontSize/2
#
#					m_pFontManager->SetSize(m_Font, littleFontSize, littleFontSize);
#					m_pFontManager->Print(fontx, radius-tickLength-littleFontSize, &buffer[0], m_Font )	
	
			else: #Otherwise it gets a short tick
				tickLength = 2.5
	
			glBegin(GL_LINES)
			glVertex2f(0,radius)
			glVertex2f(0,radius-tickLength)
			glEnd()
	
			glRotated(5.0 * indicatorDegreesPerTrueDegrees,0,0,-1)
			
		# Finally, restore the modelview matrix to what we received
		glPopMatrix()
		
		glFlush()
		self.SwapBuffers()
		#end OnDraw

