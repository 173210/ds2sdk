This readme is a guidline of how to run a example plugin on DSTWO.

1. Process of Making a Plugin can run on DSTWO
  a. developing a program utilizing ds2sdk lib, compile the program souce to 
		generate the plane binary formate execuable file. In this example, the 
		plane binary formate execuable file's name is example.bin.
  b. unsing a tool named makeplug which in ds2sdk/tools directory, to generate the
		final file which can run on DSTWO. command formate as following:
		makeplug source object
		source is the binary file generated on step 1;
		object is the final output file of this. it's name should be *.plg, 
		that is the name should be terminated with .plg suffix.
		on this step, a file which named ds2_firmware.dat must stay in the same
		directory as the makeplug tool, it is released with ds2sdk.
  c. writing a plugin configure file, it recording the information of the plugin
		which made by users, then DSTWO search this file to recognize user made
		plugin.
		the configure file's name formate is *.ini, "*" is the name of the plugin,
		it must be the same as output file's of step b.
		the configure file have the following content:
		[plug setting]
		icon=icon path
		name=name of plugin
		"icon" is the picture would be displayed on DSTWO main menu, it is optional.
		if don't exist, a default icon will be used.
		"name" is the name of the plugin, it will be displayed on DSTWO menu
		in this example, the configure file is "demoplug.ini", content are:
		[plug setting]
		icon=fat1:/_dstwoplug/demoplug.bmp
		name=DEMO PLUG
  d. copy the *.plg and *.ini file to _dstwoplug directory, if a icon picture
		used, also copy it to the place you specified in *.ini file.
		in this	example, the three files are demoplug.plg, demoplug.ini,
		demoplug.bmp, copy all of them to the _dstwoplug directory.
		then, start	DSTWO, a new plugin will be added on DSTWO main menu. in this
		example, you will see a icon named DEMO PLUG. press button A or touch
		the icon, running it.

2. Compile the Example
Source file of the example are in ds2sdk/example/src directory, if the cross-compiler
installed properly, just enter the ds2sdk/example/src directory and make, it will
produce a binary file named demoplug.bin in ds2sdk/example directory, also the 
final file demoplug.plg. then do as introdued as section 1 of this readme to run
the example plugin.
In fact, demoplug.plg and demoplug.ini already in ds2sdk/example as ds2sdk released,
also the demoplug.bmp.

3. About the Example
The example demos how to using ds2 hardware layer's API to developing programe
run on DSTWO. on DSTWO plateform, NDS just like a display, a keyboard and a mouse,
it receive video and audio data from DSTWO, and send key and touch screen 
information back to DSTWO.
the example can put a bitmap formate picture on down screen of NDS, and can play
microsoft wave file, the wave file should be 44.1KHz sampling frequence, 2 channels
stereo, 16-bit samples.
When demoplug started, the up screen is console, it display strings output by
printf function, the down screen displaying a file list of the directory, it only
list *.bmp, *.wav and sub-directory of the directory, on the right edge of the 
screen, a character 'D' will appeared if the item is a directory. press UP key
or DOWN key to view the file list, press key B return to parent directory and key
A enter the sub-directory, if the selected item is "..", press key A also return 
to parent directory.
when *.bmp file selected, demoplug enter displaying picture state, down screen
will appear the picture, on this state, press key UP will shut down backlight of
up screen and key DOWN shut down down screen, key RIGHT decrease brightness of 
screen and key LEFT increase brightness, key R shut down NDS, key L exit to 
DSTWO main menu, key B exit displaying picture state and rentrun to list file.
when *.wav file selected, demoplug enter playing wave state, dwon screen will be
black, you will heard sound. key UP increase volume and key DOWM decrease volume,
key B return to list file state.
on all state, press key L+R+X+A+B at the same time, will enter viewing console
state, press key UP or key DOWN will scroll string of up screen, key B will
switch console screen between down screen and up screen, key Y exit vieming
console state. note, on palying wave state and enter view console state, will
pause the sound.
more details, read the source.

Need more help or any advise, please email: dking024@gmail.com
Enjoy! *_^

