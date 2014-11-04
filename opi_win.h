/*** ****************************
  * HW interface functions header file
  * OP Innovations
  *
  *
  * v1.10 - 20131121	mpeng
  * --------------------------
  * 1. Added "on" and "off" mode setting for controller (UC).
  * 2. Added micro SD slot SPI interface on and off commands.
  * 3. Added opitsign trigger command (needs uSD SPI interface to be turned off
  * 4. Added battery cycling command
  *  
  * v1.05 - 20130705    mpeng
  * --------------------------
  * 1. Increased COM ports scanned from 20 to 50
  *
  * v1.00 - 20130417	mpeng
  * --------------------------
  * 1. Changed opiucd_settsrtc so it doesn't take an input timestamp. It sets the
  *  TrueSense time to the same as in the unified controller.
  *
  * v0.98 - 20130207	mpeng
  * --------------------------
  * 1. Changed OPIPKT_t so that has 2 bytes for length
  *      length and increased payload length
  * 2. Changed opiucd_getmmtsdata to opiucd_get5mmtsdata which gets 5 packets at a time
  *
  * v0.95 - 20130117	mpeng
  * --------------------------
  * 1. Added controller shutdown function
  *
  * v0.7 - 20121213	mpeng
  * --------------------------
  * 1. Split header and function definitions
  * 2. Updated names of functions
  * 3. Added some definitions that are useful, (temp one is the status length)
  *
  * v0.5 - 20121130	mpeng
  * --------------------------
  * 1. Compatible with unicon_dongle v0.64 (v0.5 hardware)
  *      - openucd, and status
  * 2. Removed format usd function
  *
  * v0.2 - 20121121	mpeng
  * --------------------------
  * 1. compatible with unicon_dongle v0.62_2
  * 2. USB access much more robust
  * 3. added many functions for setting unified controller and plugged in module
  *
  * v0.1 - 20121005	mpeng
  * --------------------------
  * 1. Original, compatible with unicon_dongle v0.54
  *
  * ****************************/

#ifndef OPI_WIN_H
#define OPI_WIN_H

/***
  * Include Files
  */
#include <windows.h>	// for com port opening related functions
#include <stdio.h>	


/***
  * Definitions
  */
#define DSNLEN		5	// Device Serial Number length in bytes
#define TSLEN		6	// Device Serial Number length in bytes
#define FWVLEN		2 	// Firmware version length in bytes
#define PDNLISTLEN	8	// Number of PDNs that can be paired
#define PDNINFOLEN  12	// Length of info bytes for each PDN stored in UC
#define WLFRMHDRLEN	2	// Length of the wireless frame header in bytes
#define WFRMHDRLEN	2   // Length of the wired frame header in bytes
#define TSFRMLEN	146 // Usual length of the wired frame with truesense data in bytes
#define EVFRMLEN    9   // Length of an wired frame with single event in bytes
#define OPIUCDSTLEN 128 // Number of bytes in a UC status packet including dataCode
#define ADCLEN		64	// Usual number of ADC samples
#define TMPLEN		1   // Length of temperature data in bytes
#define	ACCDLEN		6   // Length of accelerometer data in bytes (1 x, 1 y, 4 z)
#define ACCLEN		4   // Length of z-axis accelerometer data in bytes
#define OPIHDRLEN   512 // Number of bytes at the beginning of OPI file for header
#define SYNCBYTE	0x33	// Byte value for sync bytes
#define UCRTCFREQ   4096    // Frequency of real-time clock on Unicon
#define TSRTCFREQ   512     // Frequency of real-time clock on TrueSense


/***
  * Structure definition
  */
typedef struct OPIPKT_struct
{
	unsigned char dataCode;  	// Data Code
	unsigned short length;		// Length of payload
	unsigned char payload[1024];	// Payload array, 1024 byte array (should never be greater than 1K)
} OPIPKT_t;

// Interpreted/Fixed Received Wireless TrueSense Data
typedef struct OPIPKT_DC01_SDC01_struct
{
	unsigned char dataCode;  			// Data Code
	unsigned short length;		
	unsigned char payload[1024];

	unsigned char subDataCode;			// Sub-Data Code
	unsigned long long timeStamp;		// The timestamp is 6 bytes starting with the MSB currently in the device. The timestamp is the number of ticks of a 4096Hz clock/counter from a reference date and time of September 28, 2012 08:00:00.000.
	const char* timeStampStr;			// Human readable format of the timestamp

	unsigned char sensorPDN;			// Paired Device Number (PDN): The paired device number identifies the device the data is associated with using an unsigned byte.
	unsigned char adcDataSampleCount;	// ADC sample length information (ADC data in this packet has 62 or 64 samples)
	unsigned char wirelessDataCode;		// Signify the wireless datacode and is related to the wireless data received. If it is from the standard TrueSense, then it should usually be 1.
	bool lowBattery;					// If this is false, then the battery level is above 3.15V. If this is true, then the battery level is below 3.15V.

	unsigned char wirelessDataCorrectionCode;	// If this equala to 0, then there was no wireless data corruption and no error correction was applied. If this equals to 3, then the wireless corruption was high, and the ADC data has been blanked, the temperature and accelerometer data are extrapolated from previous samples. Intermediate values describe the level of error correction applied.
	short *adcValues;					// The physical range is -/+800uV with +800uV corresponding to +32767 and -800uV corresponding to -32768.
	
	float temperatureData;				// The temperature data, sampled every 1/8 of a second, representing the temperature if Celsius. Note that the temperature resolution is more than one degree, but can be made finer by oversampling and decimating, which are not used by default. Also, due to the IC used, the absolute accuracy of the temperature is poor (~3 degrees tolerance).

	char accelerometerX;				// The accelerometer data in the X and Y direction are sampled at 8Hz and represented in 2’s complement with a single byte in the range from -2g to +2g.
	char accelerometerY;				// The accelerometer data in the X and Y direction are sampled at 8Hz and represented in 2’s complement with a single byte in the range from -2g to +2g.
	char *accelerometerZs;				// The accelerometer data in the z-axis has the same representation as the other 2 axes, but is sampled at 32Hz, thus it has 4 times the data as the other directions.
	char accelerometerZ;				// This is the average of the 4 samples of Z accelerometer values.

	unsigned char ed;					// The ED is the lowest 7 bits of the last byte and gives the received level in a range of 0-84 with units of dB.

} OPIPKT_DC01_SDC01_t;

// UCD Status Info of slave
typedef struct OPIPKT_DC10_PDNSettings_struct
{
	unsigned long long sensorDSN;		// Device Serial Number (DSN) is 5 bytes that uniquely identifies the (TrueSense) sensor. This is a serial number programmed during production and is read MSB to LSB.
	unsigned short firmwareVersion;		// Firmware version
	unsigned char zigBeeChannel;		// The ZigBee channel, 1 byte, indicates the zigbee channel the sensor is using to send data to the controller.
	unsigned char rfOutputPower;
	unsigned char rfTransmitPower;
	unsigned char rfTimeout;
	unsigned char memoryModuleWrite;
} OPIPKT_DC10_PDNSettings_t;

// UCD Status Info of slave
typedef struct OPIPKT_DC10_struct
{
	unsigned char dataCode;				// Data Code
	unsigned short length;
	unsigned char payload[1024];

	unsigned char subDataCode;			// Sub-Data Code
	unsigned long long receiverDSN;		// Device Serial Number (DSN) is 5 bytes that uniquely identifies the unified controller. This is a serial number programmed during production and is read MSB to LSB.
	unsigned long long timeStamp;		// The timestamp is 6 bytes starting with the MSB currently in the device. The timestamp is the number of ticks of a 4096Hz clock/counter from a reference date and time of September 28, 2012 08:00:00.000.
	const char* timeStampStr;			// Human readable format of the timestamp

	unsigned char opiucd[6];			// "OPIUCD" This allows recognition of this device to be a unified controller.
	unsigned short firmwareVersion;		// Firmware version
	
	unsigned char mode;					// The Mode is 1 byte indicating the mode the unified controller is in. Normal operation has the unified controller in mode 0 most of the time.
	unsigned char associatedPDNs[8];	// PDNs are 8 bytes representing the eight Paired Device Numbers, or devices, that are paired with this unified controller. Although there is room for eight paired devices, it is not recommended to have more than 4. These should fill the first 4 slots.
	unsigned char zigBeeChannel;		// The ZigBee channel, 1 byte, indicates the zigbee channel the unified controller is using to receive data from devices.
	unsigned char uSDType;				// The uSD byte indicates the status of the uSD slot on the unified controller. If the unified controller is disabling the power of the device in the uSD slot, the 3rd LSB will be 0. If the unified controller is enabling the power of the device in the uSD slot, the 3rd LSB will be 1. If the unified controller detects a TrueSense device in the uSD slot, then the 2nd LSB will be 1, or 0 if there is no detected TrueSense device in the slot. If the unified controller detects a memory module in the uSD slot, then the LSB will be 1, or 0 if there is no detected memory module. It is possible that these 3 bits are all one since we allow a tandem configuration with TrueSense plugged into the Memory module.It is impossible to have the 3rd bit low with the first bit high because TrueSense, if it is in the slot will be shutoff and thus unrecognizable to the unified controller.
	unsigned char chargerStatus;		// Indicates 1 or 0 if the charger IC on the unified controller is actively charging (either the unified controller battery, or the truesense battery if it is plugged in).
	
	unsigned char pdnSettingsRaw[8][12];	// The PDN0-7 Info represents settings of the paired devices during pairing. These include the sensor (TrueSense) DSN, firmware version, ZigBee Channel, RF output power, RF transmit mode, RF timeout value, and memory module write.
	OPIPKT_DC10_PDNSettings_t pdnSettings[8];
} OPIPKT_DC10_t;

/***
  * Prototypes
  */
int opipkt_get_com(OPIPKT_t* pktptr, HANDLE *comportptr);
int opipkt_put_com(OPIPKT_t* pktptr, HANDLE* comportptr);
int opi_openucd_com(HANDLE* comportptr);
void opi_closeucd_com(HANDLE* comportptr);
void opipkt_dump(OPIPKT_t* pktptr);
int opiucd_status(HANDLE *comportptr, OPIPKT_t* pktptr);
int opiucd_tsstatus(HANDLE *comportptr, OPIPKT_t* pktptr);
int opiucd_settspdn(HANDLE *comportptr, int pdn);
int opiucd_settszbchan(HANDLE *comportptr, int zbChan);
int opiucd_settsrfmode(HANDLE *comportptr, int rfmode);
int opiucd_settsrftxpwr(HANDLE *comportptr, int rftxpwr);
int opiucd_settsmmwrite(HANDLE *comportptr, int mmwrite);
int opiucd_settsrftxtimeout(HANDLE *comportptr, int rftxtimeout);
int opiucd_settsrtc(HANDLE *comportptr);
int opiucd_setzbchan(HANDLE *comportptr, int zbChan);
int opiucd_setpktts(HANDLE *comportptr, int* timeStamp);
int opiucd_copytssettings(HANDLE *comportptr, int pdnslot);
int opiucd_forgettssettings(HANDLE *comportptr, int pdnslot);
int opiucd_getwltsdata(HANDLE *comportptr, OPIPKT_t* pktptr);
int opiucd_wlmeasure(HANDLE *comportptr, OPIPKT_t* pktptr);
int opiucd_evcapread(HANDLE *comportptr, OPIPKT_t* pktptr);
int opiucd_evcaperase(HANDLE *comportptr);
int opiucd_mmerasest(HANDLE *comportptr);
int opiucd_mmeraseend(HANDLE *comportptr);
int opiucd_get5mmtsdata(HANDLE *comportptr, int pktNumber, OPIPKT_t* pktptr);
int opiucd_turnmodoff(HANDLE *comportptr);
int opiucd_turnmodon(HANDLE *comportptr);
int opiucd_shutdown(HANDLE *comportptr);
int opiucd_offmode(HANDLE *comportptr);
int opiucd_onmode(HANDLE *comportptr);
int opiucd_turnusdspion(HANDLE *comportptr);
int opiucd_turnusdspioff(HANDLE *comportptr);
int opiucd_triggertsign(HANDLE *comporptr, OPIPKT_t *pktptr);
int opiucd_battcycle(HANDLE *comportptr);

#endif // OPI_WIN_H
