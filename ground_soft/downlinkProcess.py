import string
import log
import wx

def ProcessOK(data_val):
	if data_val == "":
		print "got OK"
	else:
		print "not OK"

def ProcessPitch(data_val):
	#horizon pitch [0,127]
	pitch_deg = round((int(data_val) - 64) * 90/127.0)
	#pitch_deg [-90,90]
	print "got pitch:",pitch_deg
	#self.parent.horizon.SetPitch(pitch_deg)

def ProcessRoll(data_val):
	#horizon roll [0.127]
	roll_deg = round((int(data_val) - 64) * 180/127.0)
	#roll_deg [-180,180]
	print "got roll:",roll_deg
	#self.parent.horizon.SetRoll(roll_deg)
	
def ProcessHeading(data_val):
	#compass heading [0,127]
	heading_deg = round(int(data_val) * 360/127.0)
	#heading_deg [0,360]
	print "got heading:",heading_deg
	#self.parent.compass.SetHeading(heading_deg)
	
def ProcessLatitude(data_val):
	print "got latitude:",data_val
	#self.parent.UpdateLatitude()

def ProcessLongitude(data_val):
	print "got longitude:",data_val
	#self.parent.UpdateLongitude()

def ProcessAltitude(data_val):
	print "got altitude:",data_val
	#self.parent.UpdateAltitude()

def ProcessBattery(data_val):
	#batt level [0,127]
	batt_per = round(int(data_val) * 12/127.0)
	print "got battery:",batt_per
	#batt_volt [0,12]
	#TODO: save to battery control

def ProcessFuel(data_val):
	#fuel level [0,127]
	fuel_per = round(int(data_val) * 100/127.0)
	print "got fuel:",fuel_per
	#fuel_per [0,100]
	#TODO: save to fuel control

def ProcessAirspeed(data_val):
	#airspeed [0,127]
	airspeed_knots = (data_val)
	print "got airspeed:",airspeed_knots
	# TODO: convert from total pressure to airspeed
	
def ProcessBuffer(buffer):
	data_types = ['c','a','o','s','g','f','b','q','w','1']
	data_separator = ','
	
	packets = buffer.split(data_separator)

	#dictionary of functions, with data types for keys	
	func_dict = {'1':ProcessOK,
		 'q':ProcessPitch,
		 'w':ProcessRoll,
		 'h':ProcessHeading,
		 'a':ProcessLatitude,
		 'o':ProcessLongitude,
		 'z':ProcessAltitude,
		 'b':ProcessBattery,
		 'f':ProcessFuel,
		 's':ProcessAirspeed}
	
	for p in packets:
		for dt in data_types:
			if p[0] == dt:
				dv = p[2:]
				if dv in string.ascii_letters:
					#we're compressing to ascii
					dv = ord(dv)
					func_dict[dt](dv) #run the processing function