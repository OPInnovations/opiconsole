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
    unsigned char dataCode;  	// dataCode, 1 byte
    unsigned short length;		// length of payload, 2 bytes
    unsigned char payload[1024];	// payload array, 1024 byte array (should never be greater than 1K)
} OPIPKT_t;

typedef struct OPIPKT_DC01_SDC01_struct
{
    unsigned char dataCode;  	// data code, 1 byte
    unsigned short length;		// length of payload, 2 bytes
    unsigned char payload[1024];	// payload array, 1024 byte array (should never be greater than 1K)

	unsigned char subDataCode;  // sub-data code, 1 byte
	unsigned long long timeStamp;
	const char* timeStampStr;

	unsigned char sensorPDN;
	unsigned char adcDataSampleCount; 
	unsigned char wirelessDataCode;
	bool lowBattery;

	unsigned char wirelessDataCorrectionCode;
	short *adcValues;
	
	float temperatureData;

	char accelerometerX;
	char accelerometerY;
	char *accelerometerZs;
	char accelerometerZ;

	unsigned char ed;

} OPIPKT_DC01_SDC01_t;

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
