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
			#timeout = 0 means non-blocking mode, returns immediately on read
			#timeout in seconds, accepts floats
		else:
			log.Log('e',"cannot start radio on port:"+port)
			
		return radio
		
	def StartUplinkThread(self):
		print "start uplink thread"
		if self.radio is None:
			self.radio = self.GetRadio(self.radio_port,9600)
			if not self.radio.isOpen():
				self.radio.open()
				print "restarting radio on: "+self.radio.portstr
		self.upthread = threading.Thread(target=self.UplinkThread)
		self.upthread.setDaemon(1)
		self.upalive.set()
		self.upthread.start()
		
	def StartDownlinkThread(self):
		print "start downlink thread"
		if self.radio is None:
			self.radio = self.GetRadio(self.radio_port,9600)
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
		run = 0
		
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
				if data_value > 1:	
					command_list.append(command)
					#only write commands with interesting data
				
			for out_string in command_list:
				if (self.radio is not None) and (self.radio.isOpen()):
					self.radio.write(out_string)
					print "UPLINK:",out_string[:-2] #strip \r\n	
					log.Log('u',out_string[:-2]) #strip \r\n
			time.sleep(1/30.) #run at 30 Hz
	#end UplinkThread		
	
			
	def DownlinkThread(self):	
		while(self.downalive.isSet()):
			#sleep first, so continues still have to wait
			time.sleep(1/30.) #run at 30 Hz
			buffer = ""
			#input
			try:
				buffer = self.radio.readline()
				print buffer[:-2]
			except serial.SerialException,e:
				print "radio serial error:",e
				continue
			except AttributeError,e:
				print "no radio:",e
				continue			
			if len(buffer) == 0:
				#print "no input"
				continue
			if buffer.startswith("e0"):
				#it's a joystick event acknowledge
				print "ACK"
			try:
				self.downproc.ProcessBuffer(buffer)
				
			except Exception,e:
				#catch exceptions here, so we don't freeze pyserial
				print "datalink exception:",e
				print "unrecognized data: %s" % buffer
			
			#TODO: construct NMEA sentences, mirror GPS data to gpsout
			
	#end DownlinkThread