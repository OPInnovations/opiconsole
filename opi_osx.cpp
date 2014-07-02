/*** ****************************
  * HW interface functions definition file
  * for OSX
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
  * v1.00 - 20130528	mpeng
  * --------------------------
  * 1. Release
  *
  * ****************************/

#include "opi_osx.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <string.h>


/***
  * Function definitions
  */

/***
  *	Get a specified number of bytes from the serial com port
  * with the specified number of bytes to the specified buffer in
  * a specified number of retries
  *	Inputs:
  *		comportptr, pointer to int
  *     bytep, pointer to the unsigned char buffer where data will be put
  *     bytesToRead, number of bytes to read
  *     retries, number of retries (in terms of loop)
  *	Returns:
  *		non-negative number of bytes read
  *     -1 if error
  */
int getCOMBytes(int *comportptr, unsigned char* bytep, int bytesToRead, int retries)
{
    int bytesRead;
    int bytesToGo;
    unsigned char *nextBytep;

    bytesToGo = bytesToRead;
    nextBytep = bytep;

    for(; retries; retries--)
    {
        bytesRead = read(*comportptr, (char *)nextBytep, bytesToGo);
        if(bytesRead < 0)
        {
            continue;
        }
        else if(bytesRead < bytesToGo)
        {
            nextBytep = &(nextBytep[bytesRead]);
            bytesToGo -= bytesRead;
        }
        else
        {
            bytesToGo -= bytesRead;
            break;
        }
    }

    return (bytesToRead-bytesToGo);
}


/***
  *	Get an OPI packet from the com port,
  *	Inputs:
  *		pktptr, pointer to the packet
  *		comportptr, pointer to int
  *	Returns:
  *		code:
  *			0	// valid packet
  *			1	// invalid packet
  *			2	// null packet
  *			-1 	// couldn't get packet in 20 tries
  */
int opipkt_get_com(OPIPKT_t* pktptr, int *comportptr)
{
    unsigned short i, j;
    unsigned short pktChksm, calcChksm=0;
    unsigned char tempui8;
    unsigned char buf[10];

    for (j = 0; j < 20; j++)	// 20 retries
    {
        if(getCOMBytes(comportptr, &tempui8, 1, 100000) < 1) continue;
        if(tempui8 != SYNCBYTE) continue;
        if(getCOMBytes(comportptr, buf, 4, 100000) < 4) continue;
        if(buf[0] != SYNCBYTE) continue;
        pktptr->length = buf[1] << 8;
        pktptr->length += buf[2] - 1;

        if(!(pktptr->length+1)) return 2; // empty packet
        pktptr->dataCode = buf[3];
        calcChksm += buf[3];

        if(getCOMBytes(comportptr, pktptr->payload, pktptr->length, 100000) < pktptr->length) continue;

        for(i = 0; i < pktptr->length; i++) calcChksm += pktptr->payload[i];

        if(getCOMBytes(comportptr, buf, 2, 100000) < 2) continue;
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
  *		comportptr, pointer to int
  *	Returns:
  *		code:
  *			0	// successful
  *			-1	// error
  */
int opipkt_put_com(OPIPKT_t* pktptr, int *comportptr)
{
    unsigned short i;
    unsigned short calcChksm=0;
    unsigned char buf[256];

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
    if(write(*comportptr, buf, 5) < 5) return -1;
    if(write(*comportptr, pktptr->payload, pktptr->length+2) < pktptr->length+2) return -1;
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
int opi_openucd_com(int *comportptr)
{
    int i, j, pktLen;
    char comName[80];
    unsigned char recBuf[256];
    unsigned short pktChksm, calcChksm;
    unsigned char tempui8;
    unsigned short tempui16;
    int comRetries;
    struct termios newtio;
    speed_t baud = B115200;
    DIR *dp;
    struct dirent *ep;

    comRetries = 100000;

    // compile list of devs
    dp = opendir("/dev");
    if(dp == NULL)
    {
        return -1;
    }

    for (i = 0; i < 20; i++)
    {
        // get a dev that looks like a usb cdc device
        while(ep = readdir(dp))
        {
            if(!strncmp(ep->d_name,"tty.usbmodem",12)) break;
        }
        if(!ep)
        {
            break;
        }
        sprintf(comName, "/dev/%s", ep->d_name);   // may need to change this to what OS lists USB CDC
        *comportptr=open(comName, O_RDWR | O_NDELAY);
        if(*comportptr < 0)
        {
            printf("Couldn't open port %s", comName);
            opi_closeucd_com(comportptr);
            continue;
        }

        // flush is to be done after opening
        tcflush(*comportptr, TCIOFLUSH);

        // set port parameters
        cfsetospeed(&newtio, baud);
        cfsetispeed(&newtio, baud);
        newtio.c_cflag = (newtio.c_cflag & ~CSIZE) | CS8;
        newtio.c_cflag |= CLOCAL | CREAD;
        newtio.c_cflag &= ~(PARENB | PARODD);   // no parity
        newtio.c_cflag &= ~CRTSCTS; // no hardware handshake
        newtio.c_cflag &= ~CSTOPB;  // 1 stop bit
        newtio.c_iflag = IGNBRK;
        newtio.c_iflag &= ~(IXON|IXOFF|IXANY); // no software handshake
        newtio.c_lflag=0;
        newtio.c_oflag=0;
        newtio.c_cc[VTIME]=1;
        newtio.c_cc[VMIN]=60;
        if (tcsetattr(*comportptr, TCSANOW, &newtio)!=0)
        {
            printf("tcsetattr() 1 failed");
            opi_closeucd_com(comportptr);
	    continue;
        }

        // Starting putting stuff on the comport
        tempui8 = SYNCBYTE;
        if(write(*comportptr, &tempui8, 1) < 0)
        {
            printf("Couldn't put 1st sync byte");
            opi_closeucd_com(comportptr);
            continue;
        }
        if(write(*comportptr, &tempui8, 1) < 0)
        {
            printf("Couldn't put 2nd sync byte");
            opi_closeucd_com(comportptr);
            continue;
        }

        tempui16 = 2;
        tempui8 = (tempui16 >> 8) & 0xFF;
        if(write(*comportptr, &tempui8, 1) < 0)
        {
            printf("Couldn't put length high byte");
            opi_closeucd_com(comportptr);
            continue;
        }
        tempui8 = tempui16 & 0xFF;
        if(write(*comportptr, &tempui8, 1) < 0)
        {
            printf("Couldn't put length low byte");
            opi_closeucd_com(comportptr);
            continue;
        }
        tempui8 = 0x10;
        if(write(*comportptr, &tempui8, 1) < 0)
        {
            printf("Couldn't put dataCode byte");
            opi_closeucd_com(comportptr);
            continue;
        }
        tempui8 = 0x01;
        if(write(*comportptr, &tempui8, 1) < 0)
        {
            printf("Couldn't put payload[0] byte");
            opi_closeucd_com(comportptr);
            continue;
        }
        tempui8 = 0x00;	// precalculated
        if(write(*comportptr, &tempui8, 1) < 0)
        {
            printf("Couldn't put chksm high byte");
            opi_closeucd_com(comportptr);
            continue;
        }
        tempui8 = 0x11;	// precalculated
        if(write(*comportptr, &tempui8, 1) < 0)
        {
            printf("Couldn't put chksm low byte");
            opi_closeucd_com(comportptr);
            continue;
        }

        // Put was successful, now get
        for(j = 0; j < 3; j++)
        {
            if(getCOMBytes(comportptr, &tempui8, 1, comRetries) < 1)
            {
                printf("Couldn't get 1st sync byte");
                opi_closeucd_com(comportptr);
                continue;
            }
            if(tempui8 != SYNCBYTE)
            {
                printf("1st sync byte wrong");
                opi_closeucd_com(comportptr);
                continue;
            }
            if(getCOMBytes(comportptr, &tempui8, 1, comRetries) < 1)
            {
                printf("Couldn't get 2nd sync byte");
                opi_closeucd_com(comportptr);
                continue;
            }
            if(tempui8 != SYNCBYTE)
            {
                printf("2nd sync byte wrong");
                opi_closeucd_com(comportptr);
                continue;
            }
            break;  // if got here sync bytes right
        }
        if(j == 3)
        {
            printf("Couldn't get sync bytes");
            opi_closeucd_com(comportptr);
            continue;
        }

        if(getCOMBytes(comportptr, &tempui8, 1, comRetries) < 1)
        {
            printf("Couldn't get length high byte");
            opi_closeucd_com(comportptr);
            continue;
        }
        tempui16 = tempui8 << 8;

        if(getCOMBytes(comportptr, &tempui8, 1, comRetries) < 1)
        {
            printf("Couldn't get length low byte");
            opi_closeucd_com(comportptr);
            continue;
        }

        tempui16 += tempui8;
        if(tempui16 != OPIUCDSTLEN)
        {
            printf("Packet length incorrect, Length %d", tempui16);
            opi_closeucd_com(comportptr);
            continue;
        }
        calcChksm = 0;
        pktLen = tempui8;
        for(j = 0; j < pktLen; j++)
        {
            if(getCOMBytes(comportptr, &tempui8, 1, comRetries) < 1)
            {
                break;
            }
            recBuf[j] = tempui8;
            calcChksm += recBuf[j];
        }
        if(j < pktLen)
        {
            printf("Didn't get all data in payload");
            opi_closeucd_com(comportptr);
            continue;
        }
        if(getCOMBytes(comportptr, &tempui8, 1, comRetries) < 1)
        {
            printf("Couldn't get chksm high byte");
            opi_closeucd_com(comportptr);
            continue;
        }

        pktChksm = tempui8 << 8;
        if(getCOMBytes(comportptr, &tempui8, 1, comRetries) < 1)
        {
            printf("Couldn't get chksm low byte");
            opi_closeucd_com(comportptr);
            continue;
        }

        pktChksm += tempui8;
        if(pktChksm != calcChksm)
        {
           printf("Checksum didn't match");
            opi_closeucd_com(comportptr);
            continue;
        }
        if(!((recBuf[12] == 'O') && (recBuf[13] == 'P') && (recBuf[14] == 'I')
             && (recBuf[15] == 'U') && (recBuf[16] == 'C') && (recBuf[17] == 'D')))
        {
            printf("Device not OPIUCD");
            opi_closeucd_com(comportptr);
            continue;
        }
        return 0;
    }

    (void) closedir(dp);
    // failed if got here
    return -1;
}


/***
  *	Closes the OPI device previously opened
  *	Inputs:
  *		comportptr, for closing the resource
  *	Returns:
  *		nothing
  */
void opi_closeucd_com(int *comportptr)
{
    if(comportptr == NULL)
        return;
    close(*comportptr);
    *comportptr = -1;
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

    printf("DataCode: 0x%02X, Length: %02X\nContents: ", pktptr->dataCode, pktptr->length);
    for (i = 0; i < pktptr->length; i++) printf("0x%02X ", pktptr->payload[i]);
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
int opiucd_status(int *comportptr, OPIPKT_t* pktptr)
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
int opiucd_tsstatus(int *comportptr, OPIPKT_t* pktptr)
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
int opiucd_settspdn(int *comportptr, int pdn)
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
int opiucd_settszbchan(int *comportptr, int zbChan)
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
int opiucd_settsrfmode(int *comportptr, int rfmode)
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
int opiucd_settsrftxpwr(int *comportptr, int rftxpwr)
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
int opiucd_settsmmwrite(int *comportptr, int mmwrite)
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
int opiucd_settsrftxtimeout(int *comportptr, int rftxtimeout)
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
int opiucd_settsrtc(int *comportptr)
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
int opiucd_setzbchan(int *comportptr, int zbChan)
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
int opiucd_setpktts(int *comportptr, int* timeStamp)
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
int opiucd_copytssettings(int *comportptr, int pdnslot)
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
int opiucd_forgettssettings(int *comportptr, int pdnslot)
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
int opiucd_getwltsdata(int *comportptr, OPIPKT_t* pktptr)
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
int opiucd_wlmeasure(int *comportptr, OPIPKT_t *pktptr)
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
int opiucd_evcapread(int *comportptr, OPIPKT_t* pktptr)
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
int opiucd_evcaperase(int *comportptr)
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
int opiucd_mmerasest(int *comportptr)
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
int opiucd_mmeraseend(int *comportptr)
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
int opiucd_get5mmtsdata(int *comportptr, int pktNumber, OPIPKT_t* pktptr)
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
int opiucd_turnmodoff(int *comportptr)
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
int opiucd_turnmodon(int *comportptr)
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
int opiucd_shutdown(int *comportptr)
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
int opiucd_offmode(int *comportptr)
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
int opiucd_onmode(int *comportptr)
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
int opiucd_turnusdspion(int *comportptr)
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
int opiucd_turnusdspioff(int *comportptr)
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
int opiucd_triggertsign(int *comportptr, OPIPKT_t* pktptr)
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
int opiucd_battcycle(int *comportptr)
{
    OPIPKT_t opipkt;

    opipkt.dataCode = 0x18;
    opipkt.length = 0;
    if (opipkt_put_com(&opipkt, comportptr) != 0) return -1;
    else return 0;
}
