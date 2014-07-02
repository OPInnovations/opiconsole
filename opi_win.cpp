/*** ****************************
  * HW interface functions definition file
  * OP Innovations
  *
  *
  * v1.10 - 20131121	mpeng
  * --------------------------
  * 1. Added "on" and "off" mode setting for controller (UC).
  * 2. Added micro SD slot SPI interface on and off commands.
  * 3. Added opitsign trigger command (needs uSD SPI interface to be turned off)
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
  * v0.99 - 20130216	mpeng
  * --------------------------
  * 1. Changed put_com and get_com so that they put blocks onto usb com
  * 	to speed up transfer and minimize usb transactions
  *
  * v0.98 - 20130207	mpeng
  * --------------------------
  * 1. Changed get_com and put_com and openucd_com functions so that allow 2 bytes for
  *      length and 2 bytes for checksum
  * 2. Changed opiucd_getmmtsdata to opiucd_get5mmtsdata which gets 5 packets at a time
  *
  * v0.97 - 20130128	mpeng
  * --------------------------
  * 1. Changed all functions so that if get anything other than ok from
  *      opipkt_get_com and opipkt_put_com, it is an error
  *
  * v0.95 - 20130117	mpeng
  * --------------------------
  * 1. Added controller shutdown function
  * 2. Added checking of packet number in opiucd_getmmtsdata
  *
  * v0.7 - 20121213	mpeng
  * --------------------------
  * 1. Split header and function definitions
  * 2. Updated names of functions
  *
  * ****************************/

#include "opi_win.h"

/***
  * Function definitions
  */

/***
  *	Get an OPI packet from the com port,
  *	Inputs:
  *		pktptr, pointer to the packet
  *		comportptr, pointer to handle
  *	Returns:
  *		code:
  *			0	// valid packet
  *			1	// invalid packet
  *			2	// null packet
  *			-1 	// couldn't get packet in 20 tries
  */
int opipkt_get_com(OPIPKT_t* pktptr, HANDLE *comportptr)
{
    unsigned short i, j;
    unsigned short pktChksm, calcChksm=0;
    unsigned char tempuint8;
    unsigned char buf[10];
    DWORD readIn;

    for (j = 0; j < 20; j++)	// 20 retries
    {
        if(!(ReadFile (*comportptr, &tempuint8, 1, &readIn, 0))) continue;
        if (tempuint8 != SYNCBYTE) continue;
        if(!(ReadFile (*comportptr, buf, 4, &readIn, 0))) continue;
        if (buf[0] != SYNCBYTE) continue;
        pktptr->length = buf[1] << 8;
        pktptr->length += buf[2] - 1;

        if (!(pktptr->length+1)) return 2; // empty packet
        pktptr->dataCode = buf[3];
        calcChksm += buf[3];

        if(!(ReadFile (*comportptr, pktptr->payload, pktptr->length, &readIn, 0))) continue;

        for(i = 0; i < pktptr->length; i++) calcChksm += pktptr->payload[i];

        if(!(ReadFile (*comportptr, buf, 2, &readIn, 0))) continue;
        pktChksm = buf[0] << 8;
        pktChksm += buf[1];

        if (pktChksm != calcChksm) return 1;
        else return 0;
    }

    return -1;	// if got here, then something wrong
}	


/***
  *	Put an OPI packet to the com port,
  *	Inputs:
  *		pktptr, pointer to the packet
  *		comportptr, pointer to handle
  *	Returns:
  *		code:
  *			0	// successful
  *			-1	// error
  */
int opipkt_put_com(OPIPKT_t* pktptr, HANDLE* comportptr)
{
    unsigned short i;
    unsigned short calcChksm=0;
    unsigned char buf[256];
    DWORD writeOut;

    buf[0] = SYNCBYTE;
    buf[1] = SYNCBYTE;
    buf[2] = ((pktptr->length+1) >> 8) & 0xFF;
    buf[3] = (pktptr->length+1) & 0xFF;
    buf[4] = pktptr->dataCode;
    calcChksm += pktptr->dataCode;
    for(i = 0; i < pktptr->length; i++)
    {
        calcChksm += pktptr->payload[i];
    }
    pktptr->payload[i] = calcChksm >> 8;    // use these unused bytes
    pktptr->payload[i+1] = calcChksm;       // so one less call to com port
    if(!WriteFile(*comportptr, buf, 5, &writeOut, 0)) return -1;
    if(!WriteFile(*comportptr, pktptr->payload, pktptr->length+2, &writeOut, 0)) return -1;
    return 0;
}	


/***
  *	Finds the OPI device by trying COM1 through COM20.
  *	Can't assume opened comport conforms to OPI packet protocol, so need to
  *	check each byte if written/received and if correct value
  *	Inputs:
  *		comportptr, for assigning the resource when found
  *	Returns:
  *		0 if successful
  *		-1 if not successful
  */
int opi_openucd_com(HANDLE* comportptr)
{
    int i, j, pktLen;
    DCB dcbPort;
    COMMTIMEOUTS comTimeOut;
    HANDLE comPort;
    char pcCommPort[80];
    char comName[80];
    unsigned char recBuf[256];
    unsigned short pktChksm, calcChksm;
    unsigned char tempuint8;
    unsigned short tempui16;
    DWORD readIn, writeOut;

    for (i = 1; i < 20; i++)
    {
        sprintf(comName, "\\\\.\\COM%d", i);
        strcpy(pcCommPort, comName);
        comPort = CreateFileA( pcCommPort, (GENERIC_READ | GENERIC_WRITE), 0, NULL, OPEN_EXISTING, 0, NULL );
        if (!(comPort))
        {
            printf("Couldn't open COM%d\n", i);
            CloseHandle(comPort);
            continue;
        }
        // Clear the comm port of errors and leftover junk
        ClearCommError(comPort, 0, 0);
        PurgeComm(comPort, PURGE_RXABORT|PURGE_TXABORT|PURGE_RXCLEAR|PURGE_TXCLEAR);

        if (!GetCommState(comPort,&dcbPort))
        {
            printf("Couldn't get COM%d state\n", i);
            CloseHandle(comPort);
            continue;
        }

        // Set port characteristics
        dcbPort.BaudRate = CBR_115200;
        dcbPort.ByteSize = 8;
        dcbPort.Parity = NOPARITY;
        dcbPort.StopBits = ONESTOPBIT;

        if (!SetCommState(comPort,&dcbPort))
        {
            printf("Couldn't set COM%d state\n", i);
            continue;
        }
        if (!GetCommTimeouts(comPort,&comTimeOut))
        {
            printf("Couldn't get COM%d timeouts\n", i);
            CloseHandle(comPort);
            continue;
        }
        // Set timeout characteristics to be more forgiving
        comTimeOut.ReadIntervalTimeout = 25;	// ms
        comTimeOut.ReadTotalTimeoutMultiplier = 25;
        comTimeOut.ReadTotalTimeoutConstant = 25;
        comTimeOut.WriteTotalTimeoutMultiplier = 25;
        comTimeOut.WriteTotalTimeoutConstant = 25;
        if (!SetCommTimeouts(comPort,&comTimeOut))
        {
            printf("Couldn't set COM%d timeouts\n", i);
            CloseHandle(comPort);
            continue;
        }

        // Clear the comm port of leftover junk
        PurgeComm(comPort, PURGE_RXABORT|PURGE_TXABORT|PURGE_RXCLEAR|PURGE_TXCLEAR);

        // Starting putting stuff on the comport
        tempuint8 = SYNCBYTE;
        if (!(WriteFile(comPort, &tempuint8, 1, &writeOut, 0)))	// first sync byte
        {
            printf("Couldn't put 1st sync byte in COM%d\n", i);
            CloseHandle(comPort);
            continue;
        }
        if (!(WriteFile(comPort, &tempuint8, 1, &writeOut, 0)))	// second sync byte
        {
            printf("Couldn't put 2nd sync byte in COM%d\n", i);
            CloseHandle(comPort);
            continue;
        }
        tempui16 = 2;
        tempuint8 = (tempui16 >> 8) & 0xFF;
        if (!(WriteFile(comPort, &tempuint8, 1, &writeOut, 0)))	// length
        {
            printf("Couldn't put length high byte in COM%d\n", i);
            CloseHandle(comPort);
            continue;
        }
        tempuint8 = tempui16 & 0xFF;
        if (!(WriteFile(comPort, &tempuint8, 1, &writeOut, 0)))	// length
        {
            printf("Couldn't put length low byte in COM%d\n", i);
            CloseHandle(comPort);
            continue;
        }

        tempuint8 = 0x10;
        if (!(WriteFile(comPort, &tempuint8, 1, &writeOut, 0)))	// dataCode
        {
            printf("Couldn't put dataCode byte in COM%d\n", i);
            CloseHandle(comPort);
            continue;
        }

        tempuint8 = 0x01;
        if (!(WriteFile(comPort, &tempuint8, 1, &writeOut, 0)))	// payload
        {
            printf("Couldn't put payload[0] byte in COM%d\n", i);
            CloseHandle(comPort);
            continue;
        }
        tempuint8 = 0x00;	// precalculated
        if (!(WriteFile(comPort, &tempuint8, 1, &writeOut, 0)))	// chksm high byte
        {
            printf("Couldn't put checksum byte in COM%d\n", i);
            CloseHandle(comPort);
            continue;
        }
        tempuint8 = 0x11;	// precalculated
        if (!(WriteFile(comPort, &tempuint8, 1, &writeOut, 0)))	// chksm low byte
        {
            printf("Couldn't put checksum byte in COM%d\n", i);
            CloseHandle(comPort);
            continue;
        }

        // Put was successful, now get
        for (j = 0; j < 3; j++)	// must get a syncbyte in 3 (not needed since buff cleared)
        {
            if (!(ReadFile (comPort, &tempuint8, 1, &readIn, 0))) continue;
            if ((tempuint8 != SYNCBYTE) && (!(readIn))) continue;	// first sync byte
            if (!(ReadFile (comPort, &tempuint8, 1, &readIn, 0))) continue;
            if ((tempuint8 != SYNCBYTE) && (!(readIn))) continue;	// second sync byte
            break;	// got 2 sync bytes
        }
        if (j >= 3)
        {
            CloseHandle(comPort);
            printf("Couldn't get sync bytes in COM%d\n", i);
            continue;	// did not get 2 sync bytes
        }
        if (!(ReadFile (comPort, &tempuint8, 1, &readIn, 0)))
        {
            CloseHandle(comPort);
            printf("Couldn't get length high byte in COM%d\n", i);
            continue;
        }
        tempui16 = tempuint8 << 8;
        if (!(ReadFile (comPort, &tempuint8, 1, &readIn, 0)))
        {
            CloseHandle(comPort);
            printf("Couldn't get length low byte in COM%d\n", i);
            continue;
        }
        tempui16 += tempuint8;
        // if ((tempuint8 != (32+96)) || (!(readIn)))
        if ((tempui16 != (OPIUCDSTLEN)) || (!(readIn)))
        {
            CloseHandle(comPort);
            printf("Packet length not right in COM%d, Length %d\n", i, tempui16);
            continue;	// packet length not right
        }
        calcChksm = 0;
        pktLen = tempuint8;
        for (j = 0; j < pktLen; j++)
        {
            if(!(ReadFile (comPort, &tempuint8, 1, &readIn, 0))) break;
            recBuf[j] = tempuint8;
            calcChksm += recBuf[j];
        }
        if (j < pktLen)
        {
            CloseHandle(comPort);
            printf("Didn't get all data in payload in COM%d\n", i);
            continue;	// didn't get all the data
        }
        if(!(ReadFile (comPort, &tempuint8, 1, &readIn, 0)))
        {
            CloseHandle(comPort);
            printf("Didn't get checksum high byte in COM%d\n", i);
            continue;	// didn't get checksum data
        }
        pktChksm = tempuint8 << 8;
        if(!(ReadFile (comPort, &tempuint8, 1, &readIn, 0)))
        {
            CloseHandle(comPort);
            printf("Didn't get checksum low byte in COM%d\n", i);
            continue;	// didn't get checksum data
        }
        pktChksm += tempuint8;
        if(pktChksm != calcChksm)
        {
            CloseHandle(comPort);
            printf("Checksum didn't match in COM%d\n", i);
            continue;
        }
        if(!((recBuf[12] == 'O') && (recBuf[13] == 'P') && (recBuf[14] == 'I')
             && (recBuf[15] == 'U') && (recBuf[16] == 'C') && (recBuf[17] == 'D')))
        {
            CloseHandle(comPort);
            printf("Device not OPIUCD in COM%d\n", i);
            continue;
        }
        // everything checks out if it gets here so get out
        printf("OPIUCD found on COM%d\n", i);

        *comportptr = comPort;	// Need to copy to passed pointer so that it still works
        return 0;
    }
    return -1;
}


/***
  *	Closes the OPI device previously opened
  *	Inputs:
  *		comportptr, for closing the resource
  *	Returns:
  *		nothing
  */
void opi_closeucd_com(HANDLE* comportptr)
{
    CloseHandle(*comportptr);
}


/***
  *	For debug, dump the entire contents of a packet to standard output
  *	Inputs:
  *		pktptr, pointer to the packet
  *	Returns:
  *		nothing
  */
void opipkt_dump(OPIPKT_t* pktptr)
{
    int i;

    printf("DataCode: %02X, Length: %02X\nContents: ", pktptr->dataCode, pktptr->length);
    for (i = 0; i < pktptr->length; i++) printf("%02X ", pktptr->payload[i]);
    printf("\n");
}


/***
  *	Gets the plugged in UC status: current counter value, FirmWave Version,
  *	Mode, uSD status, charger status, associated Devices and related info
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		pktptr, pointer to the packet where status will be put
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_status(HANDLE *comportptr, OPIPKT_t* pktptr)
{
    pktptr->dataCode = 0x10;
    pktptr->payload[0] = 0x01;
    pktptr->length = 1;
    if (opipkt_put_com(pktptr, comportptr) != 0) return -1;
    if (opipkt_get_com(pktptr, comportptr) != 0) return -1;
    if (!((pktptr->dataCode == 0x10) && (pktptr->length == 127))) return -1;
    else return 0;
}


/***
  *	Gets the plugged in truesense status: DSN, RTC, FirmWare Version,
  *		Paired Device Number (PDN), and other parameters
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		pktptr, pointer to the packet where status will be put
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_tsstatus(HANDLE *comportptr, OPIPKT_t* pktptr)
{
    pktptr->dataCode = 0x20;
    pktptr->payload[0] = 0x00;
    pktptr->length = 1;
    if (opipkt_put_com(pktptr, comportptr) != 0) return -1;
    if (opipkt_get_com(pktptr, comportptr) != 0) return -1;
    if (!((pktptr->dataCode == 0x20) && (pktptr->length == 19)))
        return -1;
    else
        return 0;
}


/***
  *	Assigns the plugged in truesense pdn (Paired Device Number).
  *	Valid values are 0-254. 255 is reserved for unknown device.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		pdn, the PDN to be assigned
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_settspdn(HANDLE *comportptr, int pdn)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x02;
    opipkt.payload[1] = pdn & 0xFF;
    opipkt.length = 2;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Assigns the plugged in truesense ZigBee Channel to use. Valid
  *	values are 11-26.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		zbChan, the ZigBee Channel to be assigned to module
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_settszbchan(HANDLE *comportptr, int zbChan)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x03;
    opipkt.payload[1] = zbChan & 0xFF;
    opipkt.length = 2;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Sets the plugged in truesense RF Mode:
  *     0x00 - RF and double tap&timeout off
  *	0x01 - RF on and double tap&timeout off
  *	0x02 - RF off and double tap&timeout on
  *	0x03 - RF on and double tap&timeout on
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		rfmode, mode settings in bit positions
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_settsrfmode(HANDLE *comportptr, int rfmode)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x04;
    opipkt.payload[1] = rfmode & 0xFF;
    opipkt.length = 2;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40)        return 0;
    else return -1;
}


/***
  *	Sets the plugged in truesense RF TxPwr:
  * 0 -
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		rftxpwr, mapped value to pwr
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_settsrftxpwr(HANDLE *comportptr, int rftxpwr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x05;
    opipkt.payload[1] = rftxpwr & 0x0F;
    opipkt.length = 2;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40)        return 0;
    else return -1;
}




/***
  *	Sets the plugged in truesense Memory Module Write Feature.
  *	True value means truesense will try to write to memory module.
  *	False value means truesense will NOT try to write.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		mmwrite, false or true
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_settsmmwrite(HANDLE *comportptr, int mmwrite)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x06;
    opipkt.payload[1] = mmwrite & 0xFF;
    opipkt.length = 2;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Sets the plugged in truesense RF Tx Timeout.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		rftxtimeout, value to write, 0 is never, 1 is 30min, 2 is 1hr
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_settsrftxtimeout(HANDLE *comportptr, int rftxtimeout)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x10;
    opipkt.payload[1] = rftxtimeout & 0xFF;
    opipkt.length = 2;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Sets the plugged in truesense real time counter to the same
  *	as the unified controller (unicon or UC).
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_settsrtc(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x11;
    opipkt.length = 1;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Sets the unicon zigbee channel. Valid values are 11-26.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		zbchan, ZigBee channel number 11-26
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_setzbchan(HANDLE *comportptr, int zbChan)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x08;
    opipkt.payload[1] = zbChan & 0xFF;
    opipkt.length = 2;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Sets the timer in the unicon
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		timeStamp, array of 6 bytes with MSB first representing
  *			internal 48bit timer
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_setpktts(HANDLE *comportptr, int* timeStamp)
{
    int i;
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x0F;
    for (i = 0; i < TSLEN; i++)
        opipkt.payload[1+i] = timeStamp[i];
    opipkt.length = 1+TSLEN;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Copies the settings of plugged in module to specified slot
  *	(0 thru 7 valid)
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		pdnslot, int value of pdn slot
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_copytssettings(HANDLE *comportptr, int pdnslot)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x09;
    opipkt.payload[1] = pdnslot & 0xFF;
    opipkt.length = 2;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Forget the settings of plugged in module to specified slot (0 thru 7 valid)
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		pdnslot, int value of pdn slot
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_forgettssettings(HANDLE *comportptr, int pdnslot)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x0A;
    opipkt.payload[1] = pdnslot & 0xFF;
    opipkt.length = 2;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Request wireless truesense data from unicon (received from truesense).
  *	Data format is specified in wired frame definition document.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *		pktptr, pointer to packet that will have the data, if any
  *	Returns:
  *		1 if received data
  *		0 if no data
  *		-1 if error
  */
int opiucd_getwltsdata(HANDLE *comportptr, OPIPKT_t* pktptr)
{
    pktptr->dataCode = 0x10;
    pktptr->payload[0] = 0x00;
    pktptr->length = 1;
    if (opipkt_put_com(pktptr, comportptr) != 0) return -1;
    if (opipkt_get_com(pktptr, comportptr) != 0) return -1;
    if (pktptr->dataCode == 0x40) 		return 0;
    else if (pktptr->dataCode == 0x01) 	return 1;
    else return -1;
}


/***
  *	Measure ZigBee-like signal presence and Energy Detection on current
  *	unicon channel
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *      	pktptr, pointer to packet that will be modified to have the measurement data
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_wlmeasure(HANDLE *comportptr, OPIPKT_t *pktptr)
{
    pktptr->dataCode = 0x10;
    pktptr->payload[0] = 0x10;
    pktptr->length = 1;
    if (opipkt_put_com(pktptr, comportptr) != 0) return -1;
    if (opipkt_get_com(pktptr, comportptr) != 0) return -1;
    if ((pktptr->dataCode == 0x10) && (pktptr->length == 3)) return 0;
    else return -1;
}


/***
  *	Read out captured events/tags. Format specified in wired frame definition.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *      	pktptr, pointer to packet that will be modified to have the event/tag data
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_evcapread(HANDLE *comportptr, OPIPKT_t* pktptr)
{
    pktptr->dataCode = 0x10;
    pktptr->payload[0] = 0x20;
    pktptr->length = 1;
    if (opipkt_put_com(pktptr, comportptr) != 0) return -1;
    if (opipkt_get_com(pktptr, comportptr) != 0) return -1;
    if ((pktptr->dataCode == 0x10) && (pktptr->payload[0] == 0x21)) return 0;
    else return -1;
}


/***
  *	Erase captured events
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_evcaperase(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x10;
    opipkt.payload[0] = 0x2F;
    opipkt.length = 1;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Start chip erase of memory module, does not check if done.
  *	Use a separate function to check if chip erase is done.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_mmerasest(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x2A;
    opipkt.payload[0] = 0x08;
    opipkt.length = 1;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    else return 0;
}


/***
  *	Check if end of erase of memory module, does not start
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if end of memory erase
  *		1 if no reply
  *      	-1 if error,
  */
int opiucd_mmeraseend(HANDLE *comportptr)
{
    OPIPKT_t opipkt;
    int res;

    res = opipkt_get_com(&opipkt, comportptr);
    if (res == -1) return 1;
    else if(res != 0) return -1;
    if (opipkt.dataCode == 0x40) return 0;
    else return -1;
}


/***
  *	Get 5 packets of data as specified by packet number from memory module.
  *	The packet number specifies the first packet of the 5 packets. Function
  *	checks the packet number and makes sure it is between 0 and 327679,
  *	inclusive, since these are the valid numbers.
  *	Starting packet number will always be rounded down to the nearest
  *	multiple of 5.
  *	Format of this frame is specified in the wired frame definition.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *      	pktNumber, an integer specifying the starting packet number
  *      		in memory (valid values are 0-2^18)
  *      	pktptr, pointer to packet that will be updated with data
  *	Returns:
  *		0 if data read out
  *		-1 if error
  */
int opiucd_get5mmtsdata(HANDLE *comportptr, int pktNumber, OPIPKT_t* pktptr)
{
    if ((pktNumber < 0) || (pktNumber > 327679)) return -1;
    pktNumber = (pktNumber/5)*5;    // rounding down to nearest multiple of 5
    pktptr->dataCode = 0x2A;
    pktptr->payload[0] = 0x00;
    pktptr->payload[1] = (pktNumber >> 16) & 0xFF;
    pktptr->payload[2] = (pktNumber >> 8) & 0xFF;
    pktptr->payload[3] = pktNumber & 0xFF;
    pktptr->length = 4;
    if (opipkt_put_com(pktptr, comportptr) != 0) return -1;
    if (opipkt_get_com(pktptr, comportptr) != 0) return -1;
    if ((pktptr->dataCode == 0x2A) && (pktptr->payload[0] == 0x02))
        return 0;
    else return -1;
}


/***
  *	Turn module off through unified controller
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_turnmodoff(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x20;
    opipkt.length = 1;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Turn module on through unified controller
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_turnmodon(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x21;
    opipkt.length = 1;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if (opipkt.dataCode == 0x40) 		return 0;
    else return -1;
}


/***
  *	Shutdown unified controller, will disconnect USB
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_shutdown(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x13;
    opipkt.length = 0;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    else return 0;
}


/***
  *	Set controller to off-state where it constantly turns off
  * plugged in truesense.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if ok
  *		-1 if error
  */
int opiucd_offmode(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x22;
    opipkt.length = 1;
    if(opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if(opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if(opipkt.dataCode == 0x40)   return 0;
    else return -1;
}


/***
  *	Set controller to on-state where it will turn on
  * plugged in truesense.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if ok
  *		-1 if error
  */
int opiucd_onmode(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x23;
    opipkt.length = 1;
    if(opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if(opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if(opipkt.dataCode == 0x40)   return 0;
    else return -1;
}


/***
  *	Turn controller microSD SPI interface on
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if ok
  *		-1 if error
  */
int opiucd_turnusdspion(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x0B;
    opipkt.length = 1;
    if(opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if(opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if(opipkt.dataCode == 0x40)   return 0;
    else return -1;
}


/***
  *	Turn controller microSD SPI interface off
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if ok
  *		-1 if error
  */
int opiucd_turnusdspioff(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x20;
    opipkt.payload[0] = 0x0C;
    opipkt.length = 1;
    if(opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    if(opipkt_get_com(&opipkt, comportptr) != 0) return -1;
    if(opipkt.dataCode == 0x40)   return 0;
    else return -1;
}


/***
  *	Turn controller microSD SPI interface off
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *     pktptr, pointer to modified packet
  *	Returns:
  *		0 if ok
  *		-1 if error
  */
int opiucd_triggertsign(HANDLE *comportptr, OPIPKT_t* pktptr)
{

    pktptr->dataCode = 0x20;
    pktptr->payload[0] = 0x24;
    pktptr->length = 1;
    if(opipkt_put_com(pktptr, comportptr) != 0) return -1;
    if(opipkt_get_com(pktptr, comportptr) != 0) return -1;
    if((pktptr->dataCode == 0x20) && (pktptr->payload[0] == 0x25))   return 0;
    else return -1;
}



/***
  *	Put UC in battery cycle mode. Will disconnect USB
  * and be unreachable until battery is drained.
  *	Inputs:
  *		comportptr, pointer to comport assigned to UC
  *	Returns:
  *		0 if successful
  *		-1 if error
  */
int opiucd_battcycle(HANDLE *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x18;
    opipkt.length = 0;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    else return 0;
}
