#Build script of virtualmips.
#Scons is really an excellent tool to build software!

import os,platform,SCons,glob,re,sys

#########################
#   Global Environment  #
#########################
baseEnv=Environment()

if ARGUMENTS.get('debug',0):
	baseEnv.Append(CCFLAGS = "-g")
     

helpString = """
Usage:
        scons [target] [compile options]
Targets:
        pavo:                   Build VirtualMIPS for pavo emulation
        mknandflash:            Build mknandflash tool

Compile Options:
        jit:                    Set to 0 to compile VirtualMIPS *without* JIT support(pavo only). Default 1.
        lcd:                    Set to 0 to compile VirtualMIPS *without* LCD support(pavo only). Default 1.
        mhz:                    Set to 1 to test emulator's speed. Default 0.
        debug:                  Set to 1 to compile with -g. Default 0.
        o:                      Set optimization level.Default 3.
        cc:                     Set compiler.Default "gcc". You can set cc=gcc-3.4 to use gcc 3.4.

"""


#########################
#  Project Environment  #
#########################


baseEnv.Append(INCDIR = [os.path.join(os.path.abspath('.'),'../emulator')])
baseEnv.Append(INCDIR = [os.path.join(os.path.abspath('.'),'../emulator','device')])
baseEnv.Append(INCDIR = [os.path.join(os.path.abspath('.'),'../emulator','system')])
baseEnv.Append(INCDIR = [os.path.join(os.path.abspath('.'),'../emulator','mips')])
baseEnv.Append(INCDIR = [os.path.join(os.path.abspath('.'),'../emulator','utils')])
baseEnv.Append(INCDIR = [os.path.join(os.path.abspath('.'),'../emulator','utils','net')])
baseEnv.Append(INCDIR = [os.path.join(os.path.abspath('.'),'../emulator','memory')])
baseEnv.Append(INCDIR = [os.path.join(os.path.abspath('.'),'../emulator','gdb')])
	
	
baseEnv.Append(CPPPATH = baseEnv['INCDIR'])
baseEnv.Append(LIBS = ['confuse','pthread','rt','elf'] )  
baseEnv.Append(LIBPATH =  ['/usr/lib', '/usr/local/lib'] )  
 
baseEnv.Append(CCFLAGS=  "-Wall  -fomit-frame-pointer" ) 

mhz = ARGUMENTS.get('mhz', 0)
if int(mhz):
	#test emulator's MHZ
	baseEnv.Append(CCFLAGS = "-DDEBUG_MHZ")


o = ARGUMENTS.get('o', 3)
if int(o)==3:
	baseEnv.Append(CCFLAGS=  "-O3" ) 
	baseEnv.Append(LINKFLAGS =  "-O3" ) 
elif int(o)==2:
	baseEnv.Append(CCFLAGS=  "-O2" ) 
	baseEnv.Append(LINKFLAGS =  "-O2" )
elif int(o)==1:
	baseEnv.Append(CCFLAGS=  "-O1" ) 
	baseEnv.Append(LINKFLAGS =  "-O1" ) 

cc1 = ARGUMENTS.get('cc', "gcc")
if (cc1):
	baseEnv.Replace(CC = cc1)   

   

Help(helpString)
 
def listFiles(dirs,srcexts):
	allFiles = []
	for dir in (dirs):
		path= os.path.join(os.path.abspath(dir),srcexts)
		newFiles = glob.glob(path)
		for newFile in newFiles:
			allFiles.append(newFile)
	return allFiles




if 'pavo' in COMMAND_LINE_TARGETS:
	pavo = baseEnv.Clone() 		
	pavo.Append(CPPPATH = [os.path.join(os.path.abspath('.'),'../emulator','system','jz','soc')])
	pavo.Append(CPPPATH = [os.path.join(os.path.abspath('.'),'../emulator','system','jz','pavo')])
	pavo.Append(CPPPATH = [os.path.join(os.path.abspath('.'),'../emulator','utils','sdl')])
	pavo.Append(CCFLAGS = "-DSIM_PAVO")
	lcd = ARGUMENTS.get('lcd', 1)
	if int(lcd):
		#check libsdl
		conf = Configure(pavo)
		if not conf.CheckLib('SDL'):
			print 'Did not find libSDL, exiting!'
			Exit(1)
		if not conf.CheckCHeader('SDL/SDL.h'):
			print 'SDL/SDL.h must be installed!'
			Exit(1)
		pavo = conf.Finish()
		pavo.Append(CCFLAGS = "-DSIM_LCD")
		pavo.Append(LINKFLAGS  = "-lSDL")
	
	pavo.Program('pavo',listFiles(pavo['CPPPATH'],'*.c'))
	
			
if 'mknandflash' in COMMAND_LINE_TARGETS:
	mknandflash=Environment()
	mknandflash.Program('mknandflash','../tool/mknandflash.c',LIBS = ['confuse'] )	






