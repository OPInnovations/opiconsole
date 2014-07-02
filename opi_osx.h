/*** ****************************
  * HW interface functions header file
  * for OSX
  * OP Innovations
  *
  * v1.10 - 20131121	mpeng
  * --------------------------
  * 1. Added "on" and "off" mode setting for controller (UC).
  * 2. Added micro SD slot SPI interface on and off commands.
  * 3. Added opitsign trigger command (needs uSD SPI interface to be turned off
  * 4. Added battery cycling command
  * 
  * v1.00 - 20130528	mpeng
  * --------------------------
  * 1. Release
  *
  * ****************************/

#ifndef OPI_OSX_H
#define OPI_OSX_H

/***
  * Include Files
  */
#include <stdio.h>	

/***
  * Typedef
  */
typedef int HANDLE;	// make it so code parts don't have to be changed
			// when switching from windows version SDK

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
    unsigned char payload[1024];	// payload array, 1024 byte array
                                    //(should never be greater than 1K)
} OPIPKT_t;


/***
  * Prototypes
  */
int getCOMBytes(int *comportptr, unsigned char* bytep, int bytesToRead, int retries);
int opipkt_get_com(OPIPKT_t* pktptr, int *comportptr);
int opipkt_put_com(OPIPKT_t* pktptr, int *comportptr);
int opi_openucd_com(int *comportptr);
void opi_closeucd_com(int *comportptr);
void opipkt_dump(OPIPKT_t* pktptr);
int opiucd_status(int *comportptr, OPIPKT_t* pktptr);
int opiucd_tsstatus(int *comportptr, OPIPKT_t* pktptr);
int opiucd_settspdn(int *comportptr, int pdn);
int opiucd_settszbchan(int *comportptr, int zbChan);
int opiucd_settsrfmode(int *comportptr, int rfmode);
int opiucd_settsrftxpwr(int *comportptr, int rftxpwr);
int opiucd_settsmmwrite(int *comportptr, int mmwrite);
int opiucd_settsrftxtimeout(int *comportptr, int rftxtimeout);
int opiucd_settsrtc(int *comportptr);
int opiucd_setzbchan(int *comportptr, int zbChan);
int opiucd_setpktts(int *comportptr, int* timeStamp);
int opiucd_copytssettings(int *comportptr, int pdnslot);
int opiucd_forgettssettings(int *comportptr, int pdnslot);
int opiucd_getwltsdata(int *comportptr, OPIPKT_t* pktptr);
int opiucd_wlmeasure(int *comportptr, OPIPKT_t* pktptr);
int opiucd_evcapread(int *comportptr, OPIPKT_t* pktptr);
int opiucd_evcaperase(int *comportptr);
int opiucd_mmerasest(int *comportptr);
int opiucd_mmeraseend(int *comportptr);
int opiucd_get5mmtsdata(int *comportptr, int pktNumber, OPIPKT_t* pktptr);
int opiucd_turnmodoff(int *comportptr);
int opiucd_turnmodon(int *comportptr);
int opiucd_shutdown(int *comportptr);
int opiucd_offmode(int *comportptr);
int opiucd_onmode(int *comportptr);
int opiucd_turnusdspion(int *comportptr);
int opiucd_turnusdspioff(int *comportptr);
int opiucd_triggertsign(int *comporptr, OPIPKT_t *pktptr);
int opiucd_battcycle(int *comportptr);

#endif // OPI_OSX_H
