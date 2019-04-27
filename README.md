# LBeacon

LBeacon (Location Beacon): BeDIPS uses LBeacons to deliver to users the 3D coordinates and textual descriptions of their locations. Basically, LBeacon is a low-cost, Bluetooth Smart Ready device. At deployment and maintenance times, the 3D coordinates and location description of every LBeacon are retrieved from BeDIS (Building/environment Data and Information System) and stored locally. Once initialized, each LBeacon broadcast its coordinates and location description to Bluetooth enabled mobile devices nearby within its coverage area.

Alpha version of LBeacon was demonstrated during Academia Sinica open house and will be experimented with and assessed by collaborators of the project.

## General Installation

### Installing LBeacon on Raspberry Pi

[Download](https://www.raspberrypi.org/downloads/raspbian/) Raspbian Jessie lite for the Raspberry Pi operating system and follow its installation guide.

In Raspberry Pi, install packages by running the following command:
```sh
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install -y git bluez libbluetooth-dev fuse libfuse-dev libexpat1-dev swig python-dev ruby ruby-dev libusb-1.0-0-dev default-jdk xsltproc libxml2-utils cmake doxygen
```
Download open source code for obexftp and openobex libraries:
```sh
git clone https://gitlab.com/openobex/mainline openobex
git clone https://gitlab.com/obexftp/mainline obexftp
```
To compile and install openobex and openftp libraries:
```sh
cd openobex
mkdir build
cd build
cmake ..
sudo make
sudo make install
cd ../../
cd obexftp
mkdir build
cd build
cmake ..
sudo make
sudo make install
```
Update the links/cache the dynamic loader uses:
```sh
sudo ldconfig -v
```
### Setting up for Xbee S2C
Config now can be upload by program, but still something need to set up before start using Zigbee. <br />
It is essential for Connection and Configure through Serial Port. <br />
In this project, we use `ZIGBEE TH Reg` as our main Function. The following config is for it.

1. Open XCTU (Download Link: https://www.digi.com/products/xbee-rf-solutions/xctu-software/xctu)<br />
2. Add the radio you decide to setup<br />
3. click the radio you decide to setup<br />
4. Depends on different identity/role of the zigbee(Coornidator/Endpoint), there are different parameters setting. For Coornidator (Gateway), 
	* JV (Channel Verification) = `Disable[0]`
	* CE (Coordinator Enable) = `Enable[1]`
	* DH (Destination Address High) = `0`
	* DL (Destination Address Low) = `0xFFFF`
	* AP (API Output Mode) = `API enabled[1]`
	* D6 (DIO7 Configuration) = `Disable[0]` 
	* D7 (DIO7 Configuration) = `Disable[0]`
5. For EndPoint (LBeacon),
	* JV (Channel Verification) = `Enable[1]`
	* CE (Coordinator Enable) = `Disable[0]`
	* DH (Destination Address High) = `0`
	* DL (Destination Address Low) = `0`
	* AP (API Output Mode) = `API enabled[1]`
	* D6 (DIO7 Configuration) = `Disable[0]` 
	* D7 (DIO7 Configuration) = `Disable[0]`

* Edit /boot/cmdline.txt , delete any parameter involve erial port "ttyAMA0" or "serial0". <br />
* Disabale the on-board Bluetooth for Raspberry Pi Zero W
* Edit /boot/config.txt    , add enable_uart=1 and dtoverlay=pi3-disable-bt

   
Manual For XCTU: <https://www.digi.com/resources/documentation/digidocs/PDFs/90001458-13.pdf> </br>
For the serial setting for Raspberry pi, follow the instructions in the blog of: <http://www.raspberry-projects.com/pi/pi-operating-systems/raspbian/io-pins-raspbian/uart-pins>
