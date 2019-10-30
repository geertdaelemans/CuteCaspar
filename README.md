# CuteCaspar
An experiment with CasparCG including MIDI DMX control and Raspberry PI connection.

The basic idea behind this tool is to provide a CasparCG based playlist that has some synchronised interfaces. This should enable to program a light show that plays in sync with the clips. This is achieved by sending out predefined MIDI notes that can be picked up by a DMX interface. As an extra a connection with a Raspberry PI such that the automation can be triggered by external GPIO.

Features:
* Basic playlist functionality
* Lighting scenarios can be triggered through MIDI notes
* Each clip can have a sidecar with a MIDI sequence
* A set of predefined clips that can be inserted in a running playlist
* Control of Raspberry PI board through UDP
* Scare Button: An external GPIO triggers the insertion of a (random) clip
* Communication with CasparCG through configurable ports
* Communication with Raspberry PI through configurable ports

Good to know:
* Written in Qt 5.13.1
* Only tested on Windows 10
* Tested with Raspberry PI v2
* Only works with old CasparCG releases (2.0.7)

PLEASE BEWARE

This software is currently in full development. No guarantees.

Still to do/resolve:
* Running CuteCaspar without a connection to CasparCG sometimes crashes
* Open and closing of panels does not get noticed by MainWindow
* Manipulating items in a playlist
* Need to clean-up loadNextClip()
