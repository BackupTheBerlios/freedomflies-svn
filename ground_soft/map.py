import wx
from bufferedcanvas import *
import cStringIO
import mapscript
import math

debug = True
saveToDisk = False

class MapFrame(wx.Frame):
	def __init__(self,parent,id,name,pos=(50,50),size=(500,500)):
		wx.Frame.__init__(self,parent,id,name,pos,size)
		self.parent = parent #my parent is the main app frame
		self.map = MapCanvas(self,-1,size)
		self.Bind(wx.EVT_CLOSE,self.OnClose)
		
		#mit = (236822,901998)
		#self.map.setCenter(mit)
		#self.map.setDist(1)
		#self.map.addPlaneSymbol(mit)
		
	def OnClose(self,event):
		pass
		#don't let the user close the window, hackalicious
		
class MapCanvas(BufferedCanvas):
	def __init__(self,parent,id,size):
		wx.InitAllImageHandlers()
		self.size = size
		self.parent = parent #?
		
		#setup map object
		self.map = mapscript.mapObj()
		self.map.width = size[0]
		self.map.height = size[1]
		
		self.map.setProjection('init=nad83:2001, units=m') #MA Stateline
		#self.map.setProjection('init=epsg:4326') #world GPS
		# set the output format 
		self.map.setOutputFormat(mapscript.outputFormatObj('GD/PNG') )
		self.map.interlace = False #PIL can't handle interlaced PNGs
		topo=mapscript.layerObj(None) 
		topo.name="topo"  
		topo.type=mapscript.MS_LAYER_RASTER  
		topo.connectiontype=mapscript.MS_RASTER
		
		topo.setProjection('init=nad83:2001, units=m')
		#topo.setProjection('init=epsg:4326')
		#topo.setProjection('init=epsg:26786')
		topo.status = mapscript.MS_ON
		topo.tileindex="./index.shp"
		topo.tileitem="location"
		layerNum = self.map.insertLayer(topo)
		
		BufferedCanvas.__init__(self,parent,id)
		
	def draw(self,dc):
		try:
			self.setDist(500)
			themap = self.map.draw()
			data = themap.saveToString()
			wx_image = wx.ImageFromStream(cStringIO.StringIO(data)) #convert to wx image
			bitmap = wx_image.ConvertToBitmap()
			if debug: print "MapServer thinks it's drawing..."
			if saveToDisk is True:
				f = open('map.png','wb')
				f.write(themap.getBytes())
				print "saving"
		except mapscript.MapServerError,e:
			#the map isn't ready yet
			bitmap = wx.EmptyBitmap(self.size[0],self.size[1])
			if debug: print "unable to draw, MapServerError in 'draw'."
			if debug: print e
		dc.DrawBitmap(bitmap,0,0,True)
		if debug: print "drawing"
	
	def setCenter(self,center):
		self.map.center = center
		self.update()
		#wx.BufferedCanvas.update() calls draw()
	
	def setDist(self,dist):
		"Dist in m"
		extent = mapscript.rectObj() 
		#main frame holds the currentLocation, it's referenced by
		
		currentLocation = self.parent.parent.currentLocation
		pt = mapscript.pointObj()
		print "currentLocation0 = " + str(currentLocation[0])
		print "currentLocation1 = " + str(currentLocation[0])
		pt.y = currentLocation[0] #lat
		pt.x = currentLocation[1] #lon
		
		#convert currentLocation from GPS to map coordinates
		gpsproj = mapscript.projectionObj('proj=latlong,datum=WGS84') #GPS
		
		#from stateline: http://gpsinformation.net/mapinfow.prj
		mapproj = mapscript.projectionObj('init=nad83:2001, units=m') #Stateline
		pt.project(gpsproj,mapproj)
		
		print pt
		b = (dist*math.sqrt(2)/2) #distance from center to corner
		extentquad = [pt.x-b,pt.y-b,pt.x+b,pt.y+b]
		self.map.setExtent(int(extentquad[0]),int(extentquad[1]),int(extentquad[2]),int(extentquad[3]))
		#should be close to: 233001,897999,236822,901998
		
		if debug: print "map extent:",self.map.extent
	
	def addPlaneSymbol(self,position):
		"""Adds the plane symbol at the indicated position"""
		pt = mapscript.pointObj()
		print "current_position0 = " + str(position[0])
		print "current_position1 = " + str(position[0])
		pt.y = position[1] #lat
		pt.x = position[0] #lon
		
		# project our point into the mapObj's projection 
		ddproj = mapscript.projectionObj('init=epsg:4326')
		#http://www.mass.gov/mgis/dd-over.htm
		#ddproj = mapscript.projectionObj('proj=lcc,ellps=GRS80')
		
		origproj = mapscript.projectionObj(self.map.getProjection())
		pt.project(ddproj,origproj)
		
		# create the symbol using the image 
		planeSymbol = mapscript.symbolObj('from_img') 
		planeSymbol.type = mapscript.MS_SYMBOL_PIXMAP 
		planeImg = mapscript.imageObj('images/map-plane-small.png','GD/PNG')
		#TODO: need to rotate plane to current heading
		planeSymbol.setImage(planeImg) 
		symbol_index = self.map.symbolset.appendSymbol(planeSymbol) 

		# create a shapeObj out of our location point so we can 
		# add it to the map. 
		self.routeLine = mapscript.lineObj()
		self.routeLine.add(pt)
		routeShape=mapscript.shapeObj(mapscript.MS_SHAPE_POINT)
		routeShape.add(self.routeLine) 
		routeShape.setBounds() 

		# create our inline layer that holds our location point 
		self.planeLayer = mapscript.layerObj(None)
		self.planeLayer.addFeature(routeShape) 
		self.planeLayer.setProjection(self.map.getProjection()) 
		self.planeLayer.name = "plane" 
		self.planeLayer.type = mapscript.MS_LAYER_POINT 
		self.planeLayer.connectiontype=mapscript.MS_INLINE 
		self.planeLayer.status = mapscript.MS_ON 
		self.planeLayer.transparency = mapscript.MS_GD_ALPHA 
		
		# add the image symbol we defined above to the inline layer. 
		planeClass = mapscript.classObj(None)
		planeClass.name='plane' 
		style = mapscript.styleObj(None)
		style.symbol = self.map.symbolset.index('from_img') 
		planeClass.insertStyle(style) 
		self.planeLayer.insertClass(planeClass)
		self.map.insertLayer(self.planeLayer)
		if debug: print "added plane layer, layerorder=",self.map.getLayerOrder()
	
	def getPointFromDist(self,start,dist,brngDeg):
		"""Returns a point a specified distance [km] and bearing [deg] away from start
		From http://www.movable-type.co.uk/scripts/LatLong.html
		(c) 2002-2006 Chris Veness 
		"""
		from math import asin,sin,cos,atan2,pi,sqrt
		#convert to radians
		lat1 = start[1]*pi/180
		lon1 = start[0]*pi/180 
		brng = brngDeg*pi/180
		R = 6372.795 #Earth mean radius in km
		d = dist/R #angular distance, radians

		lat2 = asin( sin(lat1)*cos(d) + cos(lat1)*sin(d)*cos(brng) )
		lon2 = lon1 + atan2( sin(brng)*sin(d)*cos(lat1),
									cos(d)-sin(lat1)*sin(lat2) )
		return (lat2*180/pi,lon2*180/pi) #convert back to degrees
		
if __name__ == '__main__':
	app = wx.PySimpleApp()
	m = MapFrame(None,-1,"Map",size=(400,300))
	m.Show(True)	
	app.MainLoop() #don't include for iPython
	