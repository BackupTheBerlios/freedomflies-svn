import wx
from bufferedcanvas import *
import cStringIO
import xmlrpclib
import mapscript

import pdb

class MapFrame(wx.Frame):
	def __init__(self,parent,id,name,pos,size):
		wx.Frame.__init__(self,parent,id,name,pos,size)
		mit = (42.359368,-71.094208) #from terraserver	
		self.map = MapCanvas(self,-1,mit,size) #default map to MIT
		self.Bind(wx.EVT_CLOSE,self.OnClose)
		
	def update(self,center):
		self.map.update(center)
		
	def OnClose(self,event):
		pass
		
class MapCanvas(BufferedCanvas):
	def __init__(self,parent,id,center,size):
		wx.InitAllImageHandlers()
		self.center = center
		self.size = size
		BufferedCanvas.__init__(self,parent,id)
	
	def draw(self,dc):
		bitmap = self.getMapCenteredAtPoint(self.center,600,self.size)
		dc.DrawBitmap(bitmap,0,0,True)
	
	def setPosition(self,center):
		self.center = center
		self.update()
	
	def getMapCenteredAtPoint(self,center,dist,size):
		"""Returns wx.Bitmap"""
		#center is (lat,lon)
		#dist is in mapunits 
		#size is (width,height)
				
		amap = mapscript.mapObj() 
		amap.height = size[1] 
		amap.width = size[0]
		# set projection to US laea     
		amap.setProjection('init=epsg:2163')
		pt = mapscript.pointObj()
		pt.x = center[1] #lon
		pt.y = center[0] #lat
		
		# project our point into the mapObj's projection 
		ddproj = mapscript.projectionObj('proj=latlong,ellps=WGS84') 
		origproj = mapscript.projectionObj(amap.getProjection()) 
		pt.project(ddproj,origproj) 
		
		# create an extent for our mapObj by buffering our projected 
		# point by the buffer distance.  Then set the mapObj's extent. 
		extent = mapscript.rectObj() 
		extent.minx = pt.x - dist 
		extent.miny = pt.y - dist 
		extent.maxx = pt.x + dist 
		extent.maxy = pt.y + dist 
		amap.setExtent(extent.minx, extent.miny, 
		               extent.maxx, extent.maxy)
		
		# set the output format to jpeg 
		outputformat = mapscript.outputFormatObj('GD/JPEG') 
		amap.setOutputFormat(outputformat) 
		
		# give the WMS client a place to put temp files
		amap.web.imagepath = "/tmp"

		# define the TerraServer WMS layer 
		layer = mapscript.layerObj(None) #creating layers with parent defined will segfault
		layer.connectiontype = mapscript.MS_WMS 
		layer.type = mapscript.MS_LAYER_RASTER 
		layer.metadata.set('wms_srs','EPSG:4326') 
		layer.metadata.set("wms_title", "USGS Topo Map") 
		ts_url = "http://terraservice.net/ogcmap.ashx?VERSION=1.1.1&SERVICE=wms&LAYERS=DRG&FORMAT=jpeg&styles=" 
		layer.connection = ts_url 
		layer.setProjection('init=epsg:4326') 
		layer.status = mapscript.MS_ON
		amap.insertLayer(layer) #use insertLayer instead !
		
		# create the symbol using the image 
		symbol = mapscript.symbolObj('from_img') 
		symbol.type = mapscript.MS_SYMBOL_PIXMAP 
		img = mapscript.imageObj('images/map-plane.png','GD/PNG') 
		symbol.setImage(img) 
		symbol_index = amap.symbolset.appendSymbol(symbol) 

		# create a shapeObj out of our address point so we can 
		# add it to the map. 
		line = mapscript.lineObj() 
		line.add(pt) 
		shape=mapscript.shapeObj(mapscript.MS_SHAPE_POINT)
		shape.add(line) 
		shape.setBounds() 

		# create our inline layer that holds our address point 
		inline_layer = mapscript.layerObj(None)
		inline_layer.addFeature(shape) 
		inline_layer.setProjection(amap.getProjection()) 
		inline_layer.name = "plane" 
		inline_layer.type = mapscript.MS_LAYER_POINT 
		inline_layer.connectiontype=mapscript.MS_INLINE 
		inline_layer.status = mapscript.MS_ON 
		inline_layer.transparency = mapscript.MS_GD_ALPHA 

		# add the image symbol we defined above to the inline layer. 
		cls = mapscript.classObj(None)
		cls.name='classname' 
		style = mapscript.styleObj(None)
		style.symbol = amap.symbolset.index('from_img') 
		cls.insertStyle(style) 
		inline_layer.insertClass(cls)
		amap.insertLayer(inline_layer)

		# draw the map
		try:
			themap = amap.draw()
		except mapscript.MapServerError,e:
			print e
			quit()
		
		data = themap.saveToString()
		wx_image = wx.ImageFromStream(cStringIO.StringIO(data)) #convert to wx image
		bitmap = wx_image.ConvertToBitmap() #convert to bitmap
		return bitmap
		
if __name__ == '__main__':
	app = wx.PySimpleApp()
	m = MapFrame(None,-1,"Map",size=(400,300))
	m.Show(1)
	