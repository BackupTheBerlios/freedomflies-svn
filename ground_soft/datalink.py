#datalink.py
import serial
import time
import threading
import wx
import log
import string
import pygame

import downlinkProcess

class radiolink(object):
	def __init__(self,parent,radio_port,gps_port):		
		self.parent = parent
		self.upthread = None
		self.upalive = threading.Event()
		self.downthread = None
		self.downalive = threading.Event()
		self.downproc = downlinkProcess.MyDownlinkProcessor(self.parent)
		self.radio_port = radio_port
		self.gps_port = gps_port
		#radio object really created in prefs.OnSave
		self.radio = None
		self.gpsout = None
	   
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
				radio.open()
				log.Log('e',"starting radio on: "+radio.portstr)
			except serial.SerialException,error:
				log.Log('e',str(error))
			#timeout = 0 means non-blocking mode, returns immediately if nothing to read
			#timeout = None means blocking mode, waits forever until read
			#timeout in seconds, accepts floats
		return radio
		
	def StartUplinkThread(self):
		log.Log('u',"start uplink thread")
		if self.radio is None:
			tmp = self.GetRadio(self.radio_port,9600)
			if tmp is None:
				log.Log('e',"radio port not set")
				return #still no radio
			else:
				self.radio = tmp
		if not self.radio.isOpen():
			self.radio.open()
			print "restarting radio on: "+self.radio.portstr
		self.upthread = threading.Thread(target=self.UplinkThread)
		self.upthread.setDaemon(1)
		self.upalive.set()
		self.upthread.start()
		
	def StartDownlinkThread(self):
		log.Log('d',"start downlink thread")
		if self.radio is None:
			tmp = self.GetRadio(self.radio_port,9600)
			if tmp is None:
				log.Log('e',"radio port not set")
				return
			else:
				self.radio = tmp
		if not self.radio.isOpen():
			self.radio.open()
			print "restarting radio on: "+self.radio.portstr
		self.downthread = threading.Thread(target=self.DownlinkThread)
		self.downthread.setDaemon(1)
		self.downalive.set()	
		self.downthread.start()
				
	def StopUplinkThread(self):
		if self.upthread is not None:
			self.upalive.clear()
			self.upthread.join()
			self.upthread = None
			if not self.downalive.isSet():
				#don't kill the radio if the downlink thread is using it
				self.radio.close()
			
	def StopDownlinkThread(self):
		if self.downthread is not None:
			self.downalive.clear()
			self.downthread.join()
			self.downthread = None
			if not self.upalive.isSet():
				#don't kill the radio if the uplink thread is using it
				self.radio.close()
				
	def UplinkThread(self):
		#save values between loops
		old_l_val = 0
		old_r_val = 0
		old_throttle_val = 0
		old_xhat = 0
		old_yhat = 0
		time_through = 0
		interval = 30.0

		#initialize no6 frequency
		#self.radio.write("f 120000\r") #finagle's constant
		
		while(self.upalive.isSet()):
			#get current stick axis values
			pygame.event.pump()
			x_val,y_val = self.parent.joystick.getPos()
			throttle_val = self.parent.joystick.getThrottle()
			x_hat,y_hat = self.parent.joystick.getHat()
			#get camera pan, tilt
			
			data_types = ['l','r','t','p','i']
			command_list = []
			data_value = 0
			
			for data_type in data_types:
				new_data = 0
				if data_type == 'l':
					#joystick left
					if ((x_val <= 0) and (abs(x_val - old_l_val)>2)):
						data_value = -1*int(x_val*255/100.0)
						old_l_val = x_val
						new_data = 1
				elif data_type == 'r':
					#joystick right
					if ((x_val >= 0) and (abs(x_val - old_r_val)>2)):
						data_value = int(x_val*255/100.0)
						old_r_val = x_val	
						new_data = 1
				elif data_type == 't':
					#throttle
					# it seems like this is pretty weird -- just for the av8r?
					if((abs(throttle_val-old_throttle_val))>2):
						data_value = int(throttle_val*255/100.0)
						old_throttle_val = throttle_val
						new_data = 1
				elif data_type == 'p':
					#camera pan
					if(x_hat != old_xhat):
						data_value = x_hat
						old_xhat = data_value
						new_data = 1
				elif data_type == 'i':
					#camera tilt
					if(x_hat != old_xhat):
						data_value = y_hat #export for write
						old_yhat = data_value
						new_data = 1
				else: #shouldn't ever get here
					data_value = 0
				
				command = string.join([data_type," ",str(data_value),"\r"],"")
				#command = string.join([data_type," ",chr(data_value),"\r"],"")
				#serial protocol is "x <ascii code>\r"
				#use chr to get string of one character with ordinal i; 0 <= i < 256
				if (new_data != 0):
					command_list.append(command)
					
			if (time_through == ((interval / 5)*1)): # This means a second has passed
				command = "2 a\r"
				command_list.append(command)
				#print time_through
			elif (time_through == ((interval / 5)*2)):
				command = "3\r"
				command_list.append(command)
				#print time_through
			elif (time_through == ((interval / 5)*3)):
				command = "2 o\r"
				command_list.append(command)
				#print time_through
			elif (time_through == ((interval / 5)*4)):
				command = "3\r"
				command_list.append(command)
				#print time_through
			else:
				command = 0
	 				
			if (time_through >=interval): 
				time_through = 0
			
			for out_string in command_list:
				if (self.radio is not None) and (self.radio.isOpen()):
					self.radio.write(out_string)
					#print "UPLINK:",out_string
					log.Log('u',out_string)
			#csik		
			time.sleep(1/interval) #run at 1 Hz
			time_through = time_through + 1
				
			#end csik
	#end UplinkThread		
	
			
	def DownlinkThread(self):	
		while(self.downalive.isSet()):
			#sleep first, so continues still have to wait
			time.sleep(1/15.) #run at 15 Hz
			buffer = ""
			#input
			try:
				buffer = self.radio.readline()
			except serial.SerialException,e:
				print "radio serial error:",e
				continue
			except AttributeError,e:
				print "no radio:",e
				continue			
			if len(buffer) == 0:
				#print "no input"
				continue
			else:
				print "DOWNLINK:",buffer[:-2]
			if buffer.startswith("e0"):
				#it's a joystick event acknowledge
				#print "ACK"
				pass
			try:
				self.downproc.ProcessBuffer(buffer)
				
			except Exception,e:
				#catch exceptions here, so we don't freeze pyserial
				print "datalink exception:",e
				print "unrecognized data: %s" % buffer
			
			#TODO: construct NMEA sentences, mirror GPS data to gpsout
			
	#end DownlinkThread