# CuteCaspar
An experiment with CasparCG including MIDI DMX control and Raspberry PI connection.

The basic idea behind this tool is to provide a CasparCG based playlist that has some synchronised interfaces. This should enable to program a light show that plays in sync with the clips. This is achieved by sending out predefined MIDI notes that can be picked up by a DMX interface. As an extra a connection with a Raspberry PI such that the automation can be triggered by external GPIO.

Features:
* Basic playlist functionality
* Creating a main playlist, a list of scares and a list of extra's (favorites)
* Lighting scenarios can be triggered through MIDI notes
* Each clip can have a sidecar with a MIDI sequence
* A set of predefined clips that can be inserted in a running playlist (extra's)
* Control of Raspberry PI board through UDP
* Scare Button: An external GPIO triggers the insertion of a (random) clip
* Communication with CasparCG through configurable ports
* Communication with Raspberry PI through configurable ports

Good to know:
* Written in Qt 6.4.1
* Only tested on Windows 10
* Tested with Raspberry PI v2
* Only tested with new CasparCG releases (2.3.x)
* Includes Python 3 example code for Raspberry PI
* Uses SQlite database
* Note/MIDI assignments imported through .csv file
* SoundScape clip runs in background for extra audio and MIDI actions

PLEASE BEWARE

This software is currently in full development. No guarantees.

Still to do/resolve:
* SoundScape clipName is hardcoded
