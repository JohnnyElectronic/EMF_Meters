# EMF Meter Replica (r2) - Updated Version!

**Blank PCB**

<img src="img/pcb_front.jpg" width="20%">  <img src="img/pcb_back.jpg" width="20%">

**Example of Assembled EMF Meter**

<img src="img/EMF_6v_front.jpg" width="20%">  <img src="../r1/img/EMF_Meter_9v_5cap_f.jpg" width="20%">

Here is the documentation and support files for building and using the Supernatural EMF Meter Replica. 
To assemble this circuit board you must be comfortable working with and soldering electronic parts.
A full assembly and usage guide is available in the "doc" folder as well as a 2 part video series.

## Technical Details

* Dimensions: 86 mm (3.46") x 95 mm (3.74") 
* Options for 9v or AA batteries
* 5 LEDS driven by an LM3914 (Jumper Option for DOT/BAR Mode)
* ATtiny85 controller
* DFPlayer module for audio
* Supports 3326,3362, or 3386 F and P series blue potentiometers
* Supports PT10 or PT15 black potentiometer
* Optional 5th yellow capacitor
* Uses a functional EECO switch to select the gain for the E-Field detector and can support a number of different EECO switches. Specifically, EECO 2700 series -02, -19, -31, -33, -41, -44 will work as is and others can be adapted to work. The PCB footprint supports 6 and 12 pin switches.
* Flexible wiring options.

This is a functional EMF meter that can detect both Electric and Magnetic fields. 
It can also be built as a simple prop by not populating the analog components. 
The meter was designed to replicate a variety of the meter configurations used in the TV series.

An ATTiny85 is used to control the DFPlayer sound board. 
Programming for the ATTiny85 can also be accomplished using an In-Circuit Serial Programming (ICSP) port to program the mounted part on the board. 
I also have available a soft touch programming cable for programming this part. 

Check out the documents folder for more details.

> [!NOTE]
> [See My YouTube Channel for examples on programming ATTiny parts using a softtouch cable](https://www.youtube.com/@Johnny_Electronic/playlists)


## Purchasing
[Visit my Tindie Store](https://www.tindie.com/stores/johnnyelectronic/)

<a href="https://www.tindie.com/stores/johnnyelectronic/?ref=offsite_badges&utm_source=sellers_JohnyElectronic&utm_medium=badges&utm_campaign=badge_medium"><img src="https://d2ss6ovg47m0r5.cloudfront.net/badges/tindie-mediums.png" alt="I sell on Tindie" width="150" height="78"></a>


## Directories

-[Schematics](schematics/)

-[Documents](doc/)

-[Images](img/)

-[Source code](../src/)

-[3D files](../3D/)

-[MP3 files](../mp3/)

## Licensing

This work is licensed under Creative Commons Attribution-ShareAlike 4.0 International. 
To view a copy of this license, visit [https://creativecommons.org/licenses/by-sa/4.0/](https://creativecommons.org/licenses/by-sa/4.0/)

Distributed as-is; no warranty is given.






