from mapscript import *   
map=mapObj() 
map.width = 1500 
map.height = 1500 

map.setProjection('proj=lcc,ellps=GRS80') 
map.setOutputFormat(outputFormatObj('GD/PNG')) 
topo=layerObj(None) 
topo.name="topo"  
topo.type=MS_LAYER_RASTER  
topo.connectiontype=MS_RASTER  
topo.data="./maps/237898.jp2"  
topo.setProjection('proj=lcc,ellps=GRS80,datum=NAD83')
topo.status = MS_ON    
layerNum = map.insertLayer(topo) 
topo.status = MS_DEFAULT
#map.setExtent(233001,897999,236822,901998)
map.setExtent(233001,897999,234822,898999)
jim = map.draw()
jim.save("okay.png")


# start with gdaltindex  index.shp q*.tif*      
# index.shp is the shape file, with elements
# index.dbf has the file names, ostensibly in the same order as the elements


                            
from mapscript import *
shpfile = shapefileObj('index.shp') 
shpfile.numshapes
shp = shpfile.getShape(shpfile.numshapes-1)  
shp.getArea() 
tilerect=shp.bounds
tilerect.maxx
tilerect.minx
tilerect.maxy
tilerect.miny

f = open("index.dbf", "rb")             
fileList=f.readlines()
f.close()
fileList = fileList[0]
fileList = fileList.replace('   ','')
fileList = fileList.replace('  ',' ') 
fileList = fileList.split(' ')
fileList.pop(0)
fileList[shpfile.numshapes-1]

MIT = pointObj(42.35830436,-71.09108681) 
shp.contains(MIT)



