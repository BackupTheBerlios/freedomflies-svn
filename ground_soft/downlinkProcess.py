import string
import log
import wx

class MyDownlinkProcessor(object):
	def __init__(self,parent):
		self.parent = parent
		
	def ProcessOK(self,data_val):
		pass
		#print "got OK"
	def ProcessPitch(self,data_val):
		#horizon pitch [0,127]
		pitch_deg = round((int(data_val) - 64) * 90/127.0)
		#pitch_deg [-90,90]
		#print "got pitch:",pitch_deg
		self.parent.horizon.SetPitch(pitch_deg)
	def ProcessRoll(self,data_val):
		#horizon roll [0.127]
		roll_deg = round((int(data_val) - 64) * 180/127.0)
		#roll_deg [-180,180]
		#print "got roll:",roll_deg
		self.parent.horizon.SetRoll(roll_deg)
	def ProcessHeading(self,data_val):
		#compass heading [0,127]
		heading_deg = round(int(data_val) * 360/127.0)
		#heading_deg [0,360]
		#print "got heading:",heading_deg
		#self.parent.compass.SetHeading(heading_deg)		
	def ProcessLatitude(self,data_val):
		direction = data_val[1] #second character
		degmin = data_val[2:-1] #strip off " +" from front, \r from back
		(degrees,minutes) = string.split(degmin,'.')
		#print "got latitude:",data_val
		self.parent.UpdateLatitude(degrees,minutes,direction)
	def ProcessLongitude(self,data_val):
		direction = data_val[1] #second character
		degmin = data_val[2:-1] #strip off " +" from front, \r from back
		(degrees,minutes) = string.split(degmin,'.')
		#print "got longitude:",data_val
		self.parent.UpdateLongitude(degrees,minutes,direction)
	def ProcessAltitude(self,data_val):
		altitude_m = data_val #TODO, convert
		#print "got altitude:",altitude_m
		self.parent.UpdateAltitude(altitude_m)
	def ProcessBattery(self,data_val):
		#batt level [0,127]
		batt_per = round(int(data_val) * 12/127.0)
		#print "got battery:",batt_per
		#batt_volt [0,12]
		#TODO: save to battery control
	def ProcessFuel(self,data_val):
		#fuel level [0,127]
		fuel_per = round(int(data_val) * 100/127.0)
		#print "got fuel:",fuel_per
		#fuel_per [0,100]
		#TODO: save to fuel control
	def ProcessAirspeed(self,data_val):
		#airspeed [0,127]
		airspeed_knots = data_val #TODO, convert
		#print "got airspeed:",airspeed_knots
		self.parent.UpdateAirspeed(airspeed_knots)
		# TODO: convert from total pressure to airspeed

	def ProcessGroundspeed(self,data_val):
		#groundspeed [0,127]
		#print "got groundspeed:",data_val
		pass
		# TODO: display to GUI
		
	def ProcessError(self,data_val):
		log.Log('e',"got no6 error"+data_val)
			
	def Dummy(self,data_val):
		#airspeed [0,127]
		print "got weird:",data_val
		# TODO: convert from total pressure to airspeed
		
	def ProcessBuffer(self,buffer):
		data_types = ['c','a','o','s','g','f','b','q','w','z','1','E']
		data_separator = ','
		
		packets = buffer.split(data_separator)
	
		#dictionary of functions, with data types for keys	
		func_dict = {'1':self.ProcessOK,
			 'q':self.ProcessPitch,
			 'w':self.ProcessRoll,
			 'c':self.ProcessHeading,
			 'a':self.ProcessLatitude,
			 'o':self.ProcessLongitude,
			 'z':self.ProcessAltitude,
			 'b':self.ProcessBattery,
			 'f':self.ProcessFuel,
			 's':self.ProcessAirspeed,
			 'g':self.ProcessGroundspeed,
			 'E':self.ProcessError}

		
		for p in packets:
			if len(p) < 3:
				#print "short packet:%s"%p
				break
			for dt in data_types:
				if p[0] == dt:
					dv = p[2:]
					if dv in string.ascii_letters:
						#we're compressing to ascii
						dv = ord(dv)
					try:
						func_dict[dt](dv) #run the processing function
					except KeyError,e:
						log.Log('e',"got unrecognized packet: %s_%s"%dt,dv)
			#end data type loop
		#end packet loop
		
		if len(buffer) > 2: #don't log garbage buffers
			log.Log('d',buffer)
	#end ProcessBuffer
	
	
#end DownlinkProcess class
