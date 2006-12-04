"""
 py2app/py2exe build script for Freedom Flies.

 Will automatically ensure that all build prerequisites are available
 via ez_setup

 Usage (Mac OS X):
     python setup.py py2app

 Usage (Windows):
     python setup.py py2exe
"""

import sys
from distutils.core import setup

NAME='Freedom Flies'
VERSION='0.1'
mainscript = 'main.py'
data_files = 'images/'
py2app_options=dict(argv_emulation=True)


if sys.platform == 'darwin':
	plist = dict(
		CFBundleIconFile=NAME,
		CFBundleName=NAME,
		CFBundleShortVersionString=VERSION,
		CFBundleGetInfoString=' '.join([NAME, VERSION]),
		CFBundleExecutable=NAME,
		CFBundleIdentifier='edu.mit.compcult.freedomflies',
	)
	
	extra_options = dict(
		setup_requires=['py2app'],
		 app=[mainscript],
		 options=dict(py2app=py2app_options,plist=plist),
		 data_files=[data_files]
	)
elif sys.platform == 'win32':
	extra_options = dict(
		setup_requires=['py2exe'],
		app=[mainscript],
		data_files=[data_files]
	)
else:
	extra_options = dict(
		# Normally unix-like platforms will use "setup.py install"
		# and install the main script as such
		scripts=[mainscript],
		data_files=[data_files]
	)

setup(
	name=NAME,
	**extra_options
)