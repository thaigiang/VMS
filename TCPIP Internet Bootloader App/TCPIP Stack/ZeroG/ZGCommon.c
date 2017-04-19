/*******************************************************************************

ZeroG Driver File for the Microchip TCP/IP Stack

Copyright © 2009 Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

*******************************************************************************/

/*********************************************************************
 *
 *  Finite State Machine and callbacks that are common to multiple
 *  types of networks (BSS and IBSS )
 *
 *********************************************************************
 * FileName:        ZGCommon.c
 * Dependencies:    None
 * Company:         ZeroG Wireless, Inc.
 *
 * Software License Agreement
 *
 * Copyright © 2009 ZeroG Wireless Inc.  All rights
 * reserved.
 *
 * ZeroG licenses to you the right to use, modify, copy,
 * distribute, and port the Software driver source files ZGCommon.c
 * and ZGCommon.h when used in conjunction with the ZeroG ZG2100 for
 * the sole purpose of interfacing with the ZeroG ZG2100.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date   		Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * SG                  12/12/08
********************************************************************/
#include "HardwareProfile.h"

#if defined(ZG_CS_TRIS)

#include <string.h> /* for memcpy */

#include "TCPIP Stack/ZGLinkMgrII.h"

#if defined (ZG_CONFIG_LINKMGRII)

#include "TCPIP Stack/ZGConsole.h"
#include "TCPIP Stack/ZGCommon.h"
#include "TCPIP Stack/ZGEasyConfig.h"

#if defined(STACK_USE_DHCP_CLIENT)
#include "TCPIP Stack/DHCP.h"
#endif


/********************/
/* Private Routines */
/********************/
static tZGBool
checkChannels(tZGU8 scanChannel)
{
    tZGU8 i;

    for ( i=0; i < ZG_GET_ACTIVE_CHANNELS(); i++ )
    {
       if (  ZG_CHANNEL(i) == scanChannel )
         return  kZGBoolTrue;
    }

    return  kZGBoolFalse;
}

#if defined ( ZG_CONFIG_CONSOLE )
tZGVoidReturn
DisplaySignalStrength(tZGU8 rssi)
{
  tZGU8 i = MAX_RSSI_TABLE_SZ-1;

  ZG_PUTRSUART("        strength = ");

  do {

    if ( ZG_RSSI_TBL_ENTRY_MIN(i) < rssi )
      ZG_PUTRSUART("#");
    else
      ZG_PUTRSUART(".");

  } while ( i-- > 0u );

  ZG_PUTRSUART("\n\r");

}
#endif

static tZGU8
SelectNetwork(tZGDataPtr pBuf)
{
    tZGU8* pDesc = &(((tZGU8*)pBuf)[kZGScanResultSZ]);
    tZGU16 i;
    tZGU8  res = 0;
    tZGScanResult* pHdr = (tZGScanResult*)pBuf;
    tZGBssDesc* pBssDesc;

    /* Load the scan result */
    ZGLibLoadConfirmBuffer(pBuf, sizeof(tZGScanResult) , 0);

    if ( pBuf == kZGDataPtrNULL )
    {
        /* Found nothing ... */
        ZG_PUTRSUART("    no network found\n\r");
        return 0;
    }
    
    for(i=0; i<pHdr->numBssDesc ; i++)
    {
        ZGLibLoadScanBuffer(pDesc, (sizeof(tZGBssDesc)),  (i * sizeof(tZGBssDesc)) );
        pBssDesc = (tZGBssDesc*) pDesc;

        /* 
         * Since scan on single channel it reports BSS on overlapping channel, skip if not on appropriate
         * channel.  Do before storing since than we only do it once 
         */
        if (pBssDesc->channel != ZG_CHANNEL(APPCXT.scanCurrentChanIdx))
            continue;

#if defined ( EZ_CONFIG_SCAN )
        buildScanList(pBssDesc);

        /* User initiated scan is informational only, do not go through selection progress */
        if (IS_USER_SCAN_INPROGRESS(APPCXT.scanStatus))
            continue;
#endif
        /* Need to check SSID since we are not filtering on it anymore in the scan request */
        if (( pBssDesc->ssidLen != ZG_SSID_LEN()) ||
          (strncmp((char*)pBssDesc->ssid, (char*)ZG_GET_SSID(), pBssDesc->ssidLen) != 0))
            continue;

        /* Check mismatch security */
        if ( ((pBssDesc->capInfo[0] & kWifiMgrCapBitPrivacy) > 0u) && (g_encType == (tZGU8)kKeyTypeNone) )
            continue;

        if ( ((pBssDesc->capInfo[0] & kWifiMgrCapBitPrivacy) == 0u) && (g_encType != (tZGU8)kKeyTypeNone) )
            continue;

        /* Need to check network type since we do not filter on it anymore */
#if !defined (ZG_CONFIG_NO_ADHOCMGRII)
        if ((ZG_GET_MODE() == kZGLMNetworkModeAdhoc) && (pBssDesc->bssType != kZGBssAdHoc))
            continue;
#endif
#if !defined (ZG_CONFIG_NO_WIFIMGRII)
        if ((ZG_GET_MODE() == kZGLMNetworkModeInfrastructure) && (pBssDesc->bssType != kZGBssInfra))
            continue;
#endif

        /* simple choice to choose the AP with the best
         * rssi. Presumably the scan was limited to AP's
         * with the same SSID. */
        if( (APPCXT.selectedBSS == 0u || (APPCXT.bssDesc.rssi < pBssDesc->rssi) ) &&
            checkChannels( pBssDesc->channel) )
        {
            memcpy( (void *) &APPCXT.bssDesc, (const void *) pBssDesc, kZGBssDescSZ);

            /* These U16 need byte swap for little endian host  */
            APPCXT.bssDesc.beaconPeriod = ZGSTOHS( APPCXT.bssDesc.beaconPeriod );
            APPCXT.bssDesc.atimWindow   = ZGSTOHS( APPCXT.bssDesc.atimWindow );
            APPCXT.selectedBSS = 1;
            res = 1;
        }
    }

    return res;
}

/*****************************/
/* Generic "Helper" Routines */
/*****************************/
tZGVoidReturn buildScanRequest( tZGScanReqPtr ptr )
{

    /* the number of usec to delay before transmitting a probe
     * request following the channel change event */
    ptr->probeDelay = ( tZGU16 ) HSTOZGS( ( tZGU16 ) kDefaultScanProbeDelay);
    /* the minimum time to spend on each channel in units
     * of TU (1024 usec) */
    ptr->minChannelTime = ( tZGU16 ) HSTOZGS(( tZGU16 ) kDefaultScanMinChanTime);
    /* the maximum time to spend on each channel in units
     * of TU (1024 usec) */
    ptr->maxChannelTime = ( tZGU16 ) HSTOZGS( ( tZGU16 ) kDefaultScanMaxChanTime);
    /* the bssid to restrict the scan too. Or ff:ff:ff:ff:ff:ff
     * to not restrict the scan by bssid */
    ptr->bssid[0] = kDefaultScanBssid_Byte0;
    ptr->bssid[1] = kDefaultScanBssid_Byte1;
    ptr->bssid[2] = kDefaultScanBssid_Byte2;
    ptr->bssid[3] = kDefaultScanBssid_Byte3;
    ptr->bssid[4] = kDefaultScanBssid_Byte4;
    ptr->bssid[5] = kDefaultScanBssid_Byte5;

    /* set ssid length to 0 to do discovery scan */
    ptr->ssidLen = 0;

    /* State based scan only scans a single channel at a time */
    ptr->chnlLen = 1;
    ptr->channelList[0] = ZG_CHANNEL(APPCXT.scanCurrentChanIdx);
}

#ifdef ZG_CONFIG_STATIC_IP
tZGVoidReturn printIPAddr( tZGVoidInput )
{

   tZGU8 ipaddr[4] = {0,0,0,0};

   TCPStackGetHostAddr(ipaddr);
   ZGConsolePrintf("\nStatic IP = %d.%d.%d.%d\n",
                     ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);

}
#endif


/*************************************/
/*  Generic FSM Next State Callbacks */
/*************************************/

enum tFSMValidStates
genericIdleNextState( tZGVoidInput )
{
   enum tFSMValidStates nextState = kSTIdle;

   /* Detect a mode state change */
   if ( ZG_GET_MODE() != ZG_GETNEXT_MODE() )
   {

#if defined ( ZG_CONFIG_CONSOLE )
      ZG_PUTRSUART("\n\r"); /* right justify console msgs */
#endif

#if !defined (ZG_CONFIG_NO_WIFIMGRII)
      if (  ZG_GETNEXT_MODE() == kZGLMNetworkModeInfrastructure )
      {
        nextState = kSTGetChipVersion;
        /* Reset the DHCP client state to reacquire an IP address - another DHCP server may */
        /* have assigned the previous IP address */
        #if defined(STACK_USE_DHCP_CLIENT)
        DHCPInit(0);
        #endif
      }
#endif

#if !defined (ZG_CONFIG_NO_ADHOCMGRII)
      if ( ZG_GETNEXT_MODE() == kZGLMNetworkModeAdhoc )
      {
        nextState = kSTEnThrottleTable;
        /* Reset the DHCP client state to reacquire an IP address - another DHCP server may */
        /* have assigned the previous IP address */
        #if defined(STACK_USE_DHCP_CLIENT)
        DHCPInit(0);
        #endif
      }
#endif

      ZG_SET_MODE( ZG_GETNEXT_MODE() );

   }

   return nextState;

}

/* This next state is designed to return NULL as a default         */
/* and let the FSM choose a static pass/fail state from the table */
enum tFSMValidStates
genericThrottleTableNext( tZGVoidInput )
{
  enum tFSMValidStates nextState = kNULL;

 /* 0xFF is a magic number for 6 bytes of broadcast MAC address */
 /* Calling memcmp() could be done, but I avoid for compares < ~10 bytes */
  if ( (ZG_MAC_ADDR_BYTE(0) == MAGIC_MAC_ADDR_BYTE1) &&
       (ZG_MAC_ADDR_BYTE(1) == MAGIC_MAC_ADDR_BYTE2) &&
       (ZG_MAC_ADDR_BYTE(2) == MAGIC_MAC_ADDR_BYTE3) &&
       (ZG_MAC_ADDR_BYTE(3) == MAGIC_MAC_ADDR_BYTE4) &&
       (ZG_MAC_ADDR_BYTE(4) == MAGIC_MAC_ADDR_BYTE5) &&
       (ZG_MAC_ADDR_BYTE(5) == MAGIC_MAC_ADDR_BYTE6) )
  {

     /* override now */
     nextState = kSTGetMacAddr;
  }

  return nextState;
}

/*********************************************/
/*  Generic FSM Management Request Callbacks */
/********************************************/
tZGU8
genericSetMacAddrRequest( void * const ptrRequest, tZGVoidInput *appOpaquePtr)
{
  tZGU8Ptr ptrMacAddr = ptrRequest;

  ptrMacAddr[0] = ZG_MAC_ADDR_BYTE(0);
  ptrMacAddr[1] = ZG_MAC_ADDR_BYTE(1);
  ptrMacAddr[2] = ZG_MAC_ADDR_BYTE(2);
  ptrMacAddr[3] = ZG_MAC_ADDR_BYTE(3);
  ptrMacAddr[4] = ZG_MAC_ADDR_BYTE(4);
  ptrMacAddr[5] = ZG_MAC_ADDR_BYTE(5);

#if defined ( ZG_CONFIG_CONSOLE )
  ZG_PUTRSUART("Set MAC addr ...\n\r");
#else
  APPCXT.FSM.bSilent =  kZGBoolTrue;
#endif


  /* This return information is required for the RAW driver */
  return ( sizeof(tZGU8) *  kZGMACAddrLen );

}

tZGU8
genericSetDomRequest( void * const ptrRequest, tZGVoidInput *appOpaquePtr)
{
  tZGU8Ptr ptrDomain = ptrRequest;

  ptrDomain[0] = ZG_GET_DOM();

#if defined ( ZG_CONFIG_CONSOLE )
  ZG_PUTRSUART("Set regional domain ...\n\r");
#else
  APPCXT.FSM.bSilent =  kZGBoolTrue;
#endif

  /* This return information is required for the RAW driver */
  return ( sizeof(tZGU8) );

}

tZGU8
genericCalcPSKRequest(void * const ptrRequest, tZGVoidInput *appOpaquePtr)
{
  tZGPskCalcReqPtr pCalc;
  pCalc = (tZGPskCalcReqPtr)ptrRequest;

  pCalc->configBits = 0;
  pCalc->phraseLen = ZG_GET_PASSPHRASE_LEN();
  pCalc->reserved = 0;
  pCalc->ssidLen = ZG_SSID_LEN();
  memcpy((void *) pCalc->ssid, (const void *) ZG_GET_SSID(), pCalc->ssidLen);
  memcpy((void *) pCalc->passPhrase, (const void *) ZG_GET_WPA_PASSPHRASE(), pCalc->phraseLen);

#if defined ( ZG_CONFIG_CONSOLE )
  ZG_PUTRSUART("Calc PSK (30 secs)...\n\r");
  APPCXT.FSM.bSilent =  kZGBoolFalse;
#else
  APPCXT.FSM.bSilent =  kZGBoolTrue;
#endif

  /* This return information is required for the RAW driver */
  return ( sizeof(tZGPskCalcReq) );

}


tZGU8
genericInstallWEPKeyRequest( void * const ptrRequest, tZGVoidInput *appOpaquePtr)
{
    tZGWEPKeyReqPtr pWep = (tZGWEPKeyReqPtr)ptrRequest;

    /* predefined RAM location in G2100 for specific key types (PSK, WEP, etc) */
    pWep->slot = tZGSecKeySlotWEPDef;
    pWep->keyLen = ZG_GET_WEP_KEY_LEN();
    pWep->defID = ZG_GET_WEP_ACTIVE_INDEX();
    pWep->ssidLen = ZG_SSID_LEN();
    memcpy( (void *) pWep->ssid, (const void *) ZG_GET_SSID(), pWep->ssidLen);
    memcpy( (void *) pWep->key, (const void *) ZG_GET_WEPKEY(0), kZGWEPKeyLenLong*4);

 #if defined ( ZG_CONFIG_CONSOLE )
    ZG_PUTRSUART("Install WEP...\n\r");
    APPCXT.FSM.bSilent =  kZGBoolFalse;
 #else
    APPCXT.FSM.bSilent =  kZGBoolTrue;
 #endif

    /* This return information is required for the RAW driver */
    return ( sizeof(tZGWEPKeyReq) );

}


tZGU8
genericInstallPSKRequest( void * const ptrRequest, tZGVoidInput *appOpaquePtr)
{

   tZGPMKKeyReqPtr pPsk = (tZGPMKKeyReqPtr)ptrRequest;
   pPsk->slot = tZGSecKeySlotPmk0;
   pPsk->ssidLen = ZG_SSID_LEN();
   memcpy( (void *) pPsk->ssid, (const void *) ZG_GET_SSID(), pPsk->ssidLen);
   memcpy( (void *) pPsk->keyData, (const void *) ZG_GET_WPAPSK(), kZGMaxPmkLen);

#if defined ( ZG_CONFIG_CONSOLE )
   ZG_PUTRSUART("Install PSK...\n\r");
   APPCXT.FSM.bSilent =  kZGBoolFalse;
#else
    APPCXT.FSM.bSilent =  kZGBoolTrue;
#endif

   /* This return information is required for the RAW driver */
   return ( sizeof(tZGPMKKeyReq) );

}

tZGU8
genericThrottleTableRequest( void * const ptrRequest, tZGVoidInput *appOpaquePtr)
{
    tZGU8Ptr ptrThrottleReq = ptrRequest;
    ptrThrottleReq[0] = kZGTxThrottleTableEnableBit;

    APPCXT.FSM.bSilent =  kZGBoolTrue;

   /* This return information is required for the RAW driver */
   return ( sizeof(tZGU8) );

}


/**********************************************/
/*  Generic FSM Management Complete Callbacks */
/**********************************************/

tZGVoidReturn
genericScanComplete (tZGU8 type, tZGDataPtr fourByteHeader, tZGDataPtr pBuf,
                     tZGU16 len, tZGVoidInput *appOpaquePtr)
{
    tZGU8 result = fourByteHeader[0];

#if defined(EZ_CONFIG_SCAN)
    /* Use state status for non-user initiated scans because */
    /* these are running in context of the state machine */
    if (!IS_USER_SCAN_INPROGRESS(APPCXT.scanStatus))
#endif
        APPCXT.FSM.stateStatus = kRETRY;

    if( result == (tZGU8)kZGResultSuccess ) {
        /* Channel scan successful, coalate reults and pick candidate */
        SelectNetwork(pBuf); /* Select connect candidate */
    } else
        ZG_PUTRSUART(" iteration of scan failed???\n\r");

    /* Regardless of success, we will march forward */
    if (++APPCXT.scanCurrentChanIdx == ZG_GET_ACTIVE_CHANNELS()) {
       /* We have come to end of scan... */
#if defined(EZ_CONFIG_SCAN)
       if (IS_USER_SCAN_INPROGRESS(APPCXT.scanStatus)) {
#if defined ( ZG_CONFIG_CONSOLE )
           DisplayScanList();
#endif
           CLEAR_USER_SCAN_INPROGRESS(APPCXT.scanStatus);
       } else 
#endif
       {
           if (APPCXT.selectedBSS)
               APPCXT.FSM.stateStatus = kSUCCESS; /* Found connect candidate */
           else
               APPCXT.FSM.stateStatus = kFAILURE; /* Did not find connect candidate */
       }
       CLEAR_SCAN_INPROGRESS(APPCXT.scanStatus);
    } 
}

tZGVoidReturn
genericGetChipVerComplete(tZGU8 type, tZGDataPtr fourByteHeader, tZGDataPtr pBuf,
                          tZGU16 len, tZGVoidInput *appOpaquePtr)
{
   tZGU8 result = fourByteHeader[0];
   APPCXT.FSM.stateStatus = kFAILURE;

   if( result == (tZGU8)kZGResultSuccess )
   {

      ZGLibLoadConfirmBuffer(pBuf, sizeof(tZGU8)*2 , 0);

      APPCXT.romVersion  = (tZGU8) pBuf[kZGGetMACParamCnfSZ];
      APPCXT.patchVersion  = (tZGU8) pBuf[kZGGetMACParamCnfSZ+1];

#if defined ( ZG_CONFIG_CONSOLE )
      sprintf( (char *) g_ConsoleContext.txBuf,
                 "ZeroG Firmware Version 0x%x \n\r",
                (tZGU16) (((tZGU16)APPCXT.romVersion << 8)| APPCXT.patchVersion)  );

      ZG_PUTSUART( (char *) g_ConsoleContext.txBuf );
#endif

      APPCXT.FSM.stateStatus = kSUCCESS;

   }

}


tZGVoidReturn
genericComplete (tZGU8 type, tZGDataPtr fourByteHeader, tZGDataPtr pBuf,
                 tZGU16 len, tZGVoidInput *appOpaquePtr)
{
   tZGU8 result = fourByteHeader[0];

   APPCXT.FSM.stateStatus = kFAILURE;

   if( result == (tZGU8)kZGResultSuccess )
   {
      if ( !APPCXT.FSM.bSilent )
        ZG_PUTRSUART("    succeeded\n\r");

      APPCXT.FSM.stateStatus = kSUCCESS;
   }
   /* Failure */
   else if ( !APPCXT.FSM.bSilent )
   {

     ZG_PUTRSUART("    failed\n\r");

#if defined ( ZG_CONFIG_CONSOLE )
     sprintf( (char *) g_ConsoleContext.txBuf,
             "    result code = %u'd\n\r", result);
     ZG_PUTSUART( (char *) g_ConsoleContext.txBuf );
#endif

   }

}

tZGVoidReturn
genericCalcPSKComplete (tZGU8 type, tZGDataPtr fourByteHeader, tZGDataPtr pBuf,
                        tZGU16 len, tZGVoidInput *appOpaquePtr)
{
   tZGU8 result = fourByteHeader[0];

   APPCXT.FSM.stateStatus = kFAILURE;

   if( result == (tZGU8)kZGResultSuccess )
   {
      tZGPskCalcCnfPtr pKey = (tZGPskCalcCnfPtr)pBuf;

      ZGLibLoadConfirmBuffer(pBuf, sizeof(tZGPskCalcCnf), 0);

      ZG_SET_WPAPSK( pKey->psk );

      if ( !APPCXT.FSM.bSilent )
        ZG_PUTRSUART("    succeeded\n\r");

      APPCXT.FSM.stateStatus = kSUCCESS;
   }
   /* Failure */
   else if ( !APPCXT.FSM.bSilent )
   {
      ZG_PUTRSUART("    failed\n\r");
   }

}


tZGVoidReturn
genericSetMacAddrComplete (tZGU8 type, tZGDataPtr fourByteHeader, tZGDataPtr pBuf,
                           tZGU16 len, tZGVoidInput *appOpaquePtr)
{
   tZGU8 result = fourByteHeader[0];

   APPCXT.FSM.stateStatus = kFAILURE;

   if ( result == (tZGU8)kZGResultSuccess)
   {

#if defined ( ZG_CONFIG_CONSOLE )

      sprintf( (char *) g_ConsoleContext.txBuf,
              "    mac = %02X:%02X:%02X:%02X:%02X:%02X\n\r",
                           ZG_MAC_ADDR_BYTE(0),
                           ZG_MAC_ADDR_BYTE(1),
                           ZG_MAC_ADDR_BYTE(2),
                           ZG_MAC_ADDR_BYTE(3),
                           ZG_MAC_ADDR_BYTE(4),
                           ZG_MAC_ADDR_BYTE(5) );

      ZG_PUTSUART( (char *) g_ConsoleContext.txBuf );

#endif


      APPCXT.FSM.stateStatus = kSUCCESS;
   }
   else
   {
     ZGErrorHandler((ROM FAR char*) "Set MAC addr");
   }

}

tZGVoidReturn
genericGetMacAddrComplete (tZGU8 type, tZGDataPtr fourByteHeader, tZGDataPtr pBuf,
                           tZGU16 len, tZGVoidInput *appOpaquePtr)
{
   tZGU8 result = fourByteHeader[0];

   APPCXT.FSM.stateStatus = kFAILURE;

   if ( result == (tZGU8)kZGResultSuccess)
   {

       ZGLibLoadConfirmBuffer(pBuf, sizeof(tZGU8) * kZGMACAddrLen, 0);

       ZG_SET_MAC_ADDR(pBuf+4);

       APPCXT.FSM.stateStatus = kSUCCESS;
   }
   else
   {
     ZGErrorHandler((ROM FAR char*) "Get MAC addr");
   }

}


tZGVoidReturn
genericSetDomComplete (tZGU8 type, tZGDataPtr fourByteHeader, tZGDataPtr pBuf,
                       tZGU16 len, tZGVoidInput *appOpaquePtr)
{
   tZGU8 result = fourByteHeader[0];

   APPCXT.FSM.stateStatus = kFAILURE;

   if ( result == (tZGU8)kZGResultSuccess)
   {

#if defined ( ZG_CONFIG_CONSOLE )

      sprintf( (char *) g_ConsoleContext.txBuf,
#if defined( __18CXX)
               "    region = %HS \n\r",
#else
               "    region = %s \n\r",
#endif
               (FAR ROM char *) ZG_GET_CUR_DOM_STR() );


      ZG_PUTSUART( (char *) g_ConsoleContext.txBuf );
#endif

      APPCXT.FSM.stateStatus =  kSUCCESS;
   }
   else
   {
      ZGErrorHandler((ROM FAR char*) "failed Reg Domain set");
   }

}


/*****************************************************************************
 * FUNCTION: genericIndicate
 *
 * RETURNS: void
 *
 * PARAMS:    tZGU8 type - the Management indication type. possible value can
 *                be found in ZGDriverIfaceApp.h.
 *            tZGDataPtr fourByteHeader - first four bytes of the message which
 *                is returned by all indication messages. Interpretation is
 *                message dependant.
 *            tZGDataPtr buf - optional parameter that provides space for
 *                additional message data if required. the receiver is
 *                responsible for recycling this buffer when done. this
 *                pointer may be NULL.
 *            tZGU16 len - the number of bytes stored in buf.
 *
 * NOTES: Called by the ZGLibrary after the application has registered interest
 *        in ZGDriver notifications, such as link connection lost.
 *****************************************************************************/
tZGVoidReturn
genericIndicate(tZGU8 type, tZGDataPtr fourByteHeader, tZGDataPtr pBuf, tZGU16 len)
{
    tZGU16 status;

    switch(type)
    {
    case kZGMgtIndDisconnect:
        /* A deauth indication occurs when the AP has announced
             * to our chip that the connection is no longer valid.  It becomes
             * the responsibility of the wifiManager to determine what to do
             * in that situation */
        APPCXT.bConnected = kZGBoolFalse;
        /* Make the selected network invalid */
        APPCXT.selectedBSS = 0;
        break;

    case kZGMgtIndConnStatus:

        status = (fourByteHeader[0] << 8) | fourByteHeader[1];
        if(status == (tZGU8)kZGConnStatusLost)
        {
            /* the criteria setup by the host that indicates when an AP
             * is lost has occurred. The code should take appropriate action
             * (notify user, try to reconnect ...). The chip is still
             * considered to be connected until the Host sends a disconnect
             * request or a deauth frame is received from the AP. */
            APPCXT.bConnLost = 1;
            APPCXT.bConnFound = 0;
            /* Make the selected network invalid */
            APPCXT.selectedBSS = 0;
        }
        else if (status == (tZGU8)kZGConnStatusFound)
        {
            APPCXT.bConnLost = 0;
            APPCXT.bConnFound = 1;
        }
        break;

    default:
        /* error condition un-recognized type */
        break;
    }

}



#endif /* ZG_CONFIG_LINKMGRII */


#endif // #if defined(ZG_CS_TRIS)
