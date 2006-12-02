#datalink.py
import serial
import time
import threading
import wx
import log
import string
import pygame

class radiolink(object):
	def __init__(self,parent,radio_port,gps_port):		
		self.parent = parent
		self.upthread = None
		self.upalive = threading.Event()
		self.downthreat = None
		self.downalive = threading.Event()
		self.radio_port = radio_port
		self.gps_port = gps_port
		self.radio = self.GetRadio(self.radio_port,9600)
		self.gpsout = self.GetRadio(self.gps_port,4800)
		#radio object really created in prefs.OnSave
			
		try:
			self.stik = self.parent.stik
		except AttributeError:
			pass
			#joystick object created in calibration window
		#error handling for missing ports done in main
		
		
	def GetRadio(self,port,baud):
		radio = None #init
		if port != "":
			try:
				radio = serial.Serial(port,baud,timeout = 0)
				log.Log('e',"starting radio on: "+radio.portstr)
			except serial.SerialException,error:
				log.Log('e',str(error))
			#timeout = 0 means non-blocking mode, returns immediately on read
			#timeout in seconds, accepts floats
		else:
			log.Log('e',"cannot start radio on port:"+port)
		return radio
		
	def StartUplinkThread(self):
		print "start uplink thread"
		if self.radio is None:
			self.radio = self.GetRadio(self.radio_port,9600)
		self.radio.open()
		self.upthread = threading.Thread(target=self.UplinkThread)
		self.upthread.setDaemon(1)
		self.upalive.set()
		self.upthread.start()
		
	def StartDownlinkThread(self):
		print "start downlink thread"
		if self.radio is None:
			self.radio = self.GetRadio(self.radio_port,9600)
		self.radio.open()
		self.downthread = threading.Thread(target=self.DownlinkThread)
		self.downthread.setDaemon(1)
		self.downalive.set()	
		self.downthread.start()
				
	def StopUplinkThread(self):
		if self.upthread is not None:
			self.radio.close()
			self.upalive.clear()
			self.upthread.join()
			self.upthread = None
			
	def StopDownlinkThread(self):
		if self.downthread is not None:
			self.radio.close()
			self.downalive.clear()
			self.downthread.join()
			self.downthread = None
	
	def UplinkThread(self):
		run = 0
		
		if not self.radio.isOpen():
			self.radio.open()
		
		while(self.upalive.isSet()):
			#get current stick axis values
			pygame.event.pump()
			x_val,y_val = self.parent.joystick.getPos()
			throttle_val = self.parent.joystick.getThrottle()
			#get camera pan, tilt
			
			data_types = ['l','r','t','p','i']
			command_list = []
			
			for data_type in data_types:
				if data_type == 'l':
					#joystick left
					if x_val < 0:
						data_value = -1*int(x_val*127/100.0)
					else:
						data_value = 0
				elif data_type == 'r':
					#joystick right
					if x_val > 0:
						data_value = int(x_val*127/100.0)
					else:
						data_value = 0
				elif data_type == 't':
					#throttle
					data_value = int(throttle_val*127/100.0)
				#TODO: add camera pan, tilt
				else:
					data_value = 0
				
				command = string.join([data_type,' ',str(data_value),'\r\n'],'')
				#command = string.join([data_type,' ',chr(data_value),'\r\n'],'')
				#serial protocol is "x <ascii code>\r"
				#use chr to get string of one character with ordinal i; 0 <= i < 256
				if data_value != 0:	
					command_list.append(command)
					#only write commands with valid data
				
			for out_string in command_list:
				if self.radio is not None:
					self.radio.write(out_string)
					print "UPLINK:",out_string
				else:
					print "X "+out_string
			time.sleep(1/30.) #run at 30 Hz
	#end UplinkThread		
	
	#note, cannot use log in this thread, due to non-threadsafe wx widgets		
	def DownlinkThread(self):
		if not self.radio.isOpen():
			self.radio.open()
	
		while(self.downalive.isSet()):
			buffer = ""
			#input
			if self.radio is not None:
				buffer = self.radio.readline()
			else:
				print "no radio"
				continue

			try: #catch type conversion errors
				data_type = buffer[0]
				
				if len(buffer) == 0:
					print "no input"
					continue
				
				buffer = buffer[:-2] #drop \r\n
				raw_data_val = buffer[2:]
				
				if len(raw_data_val) == 0:
					print "bad data_val: %s" % raw_data_val
					continue
				
				if buffer[2] in string.ascii_letters:
					data_val = ord(raw_data_val)
					#use ord to get integer ordinal of one character string
				if len(buffer) > 5:
					#probably GPS string
					data_val = str(raw_data_val)
				else:
					data_val = int(raw_data_val)
	
				if data_type == '1':
					#OK signal
					print "got OK"
					#some gui handle
				elif data_type == 'q':
					#horizon pitch [0,127]
					pitch_deg = data_val
					#pitch_deg = (data_val - 64) * 90/127.0
					#pitch_deg [-90,90]
					print "got pitch:",pitch_deg
					#self.parent.horizon.SetPitch(pitch_deg)
				elif data_type == 'w':
					#horizon roll [0.127]
					roll_deg = data_val
					#roll_deg = (data_val - 64) * 180/127.0
					#roll_deg [-180,180]
					print "got roll:",roll_deg
					#self.parent.horizon.SetRoll(roll_deg)
				elif data_type == 'h':
					#compass heading [0,127]
					heading_deg = (data_val) * 360/127.0
					#heading_deg [0,360]
					print "got heading:",heading_deg
					#self.parent.compass.SetHeading(heading_deg)
				elif data_type == 'a':
					print "got latitude:",data_val
					#self.parent.UpdateLatitude()
				elif data_type == 'o':
					print "got longitude:",data_val
					#self.parent.UpdateLongitude()
				elif data_type == 'z':
					print "got altitude:",data_val
					#self.parent.UpdateAltitude()
				elif data_type == 'b':
					#batt level [0,127]
					batt_per = (data_val) * 12/127.0
					print "got battery:",batt_per
					#batt_volt [0,12]
					#TODO: save to battery control
				elif data_type == 'f':
					#fuel level [0,127]
					fuel_per = (data_val) * 100/127.0
					print "got fuel:",fuel_per
					#fuel_per [0,100]
					#TODO: save to fuel control
				elif data_type == 's':
					#airspeed [0,127]
					airspeed_knots = (data_val)
					print "got airspeed:",airspeed_knots
					# TODO: convert from total pressure to airspeed
			except Exception,e:
				print "datalink exception",e
				print "unrecognized data: %s" % buffer
				print "data_type :%s:"% data_type
				print "data_val :%s:"% raw_data_val
				
			#TODO: construct NMEA sentences, mirror GPS data to gpsout
					
			time.sleep(1/30.) #run at 30 Hz
	#end DownlinkThread