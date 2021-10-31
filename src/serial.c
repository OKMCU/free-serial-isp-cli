/*******************************************************************************
 * Copyright (c) 2021-2022, OKMCU Development Team 
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Website: http://www.okmcu.com
 *
 * File Description: Template description.
 *
 * Change Logs:
 * Date         Author       Notes    
 * 2021-10-30   Wentao SUN   first version
 * 
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "inc/serial.h"
/* Private define ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

int32_t com_open(const char *port, com_param_t *param, com_handle_t *handle)
{
    uint8_t port_idx;
    //DWORD event_mask;
    HANDLE hdl;
    DCB dcbSerialParam = {0};

    static const BYTE stopbits[] = {
        ONESTOPBIT,
        ONE5STOPBITS,
        TWOSTOPBITS
    };

    static const BYTE parity[] = {
        NOPARITY,
        ODDPARITY,
        EVENPARITY,
        MARKPARITY,
        SPACEPARITY
    };

    hdl = CreateFile(port,                            // Serial Port name
                     GENERIC_READ | GENERIC_WRITE,    // Read/Write
                     0,                               // No Sharing
                     NULL,                            // No Security
                     OPEN_EXISTING,                   // Open existing port only
                     0,                               // Non Overlapped I/O
                     NULL);                           // Null for Comm Devices

    if(hdl == INVALID_HANDLE_VALUE)
        return -1;


    dcbSerialParam.DCBlength = sizeof(dcbSerialParam);

    if(GetCommState(hdl, &dcbSerialParam) == FALSE)
    {
        CloseHandle(hdl);
        return -1;
    }

    dcbSerialParam.BaudRate = param->baudrate;
    dcbSerialParam.ByteSize = param->bytesize;

    if(param->stopbits < sizeof(stopbits)/sizeof(stopbits[0]))
    {
        dcbSerialParam.StopBits = stopbits[param->stopbits];
    }
    else
    {
        CloseHandle(hdl);
        return -1;
    }

    if(param->parity < sizeof(parity)/sizeof(parity[0]))
    {
        dcbSerialParam.Parity = parity[param->parity];
    }
    else
    {
        CloseHandle(hdl);
        return -1;
    }

    if(SetCommState(hdl, &dcbSerialParam) == FALSE)
    {
        CloseHandle(hdl);
        return -1;
    }

/*
    if(SetCommMask(hdl, EV_RXCHAR) == FALSE)
    {
        CloseHandle(hdl);
        return -1;
    }
*/

    *handle = hdl;
    return 0;
}

int32_t com_send(com_handle_t handle, const uint8_t *buf, size_t size)
{
    DWORD dwtxcnt;
    DCB dcbSerialParam = {0};
    COMMTIMEOUTS timeouts = {0};

    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if(SetCommTimeouts(handle, &timeouts) == FALSE)
        return -1;

    while(size)
    {
        if(WriteFile(handle, buf, size, &dwtxcnt, NULL) == FALSE)
            return -1;
        buf += dwtxcnt;
        size -= dwtxcnt;
    }

    return 0;
}

int32_t com_recv(com_handle_t handle, uint8_t *buf, size_t size, size_t *rxcnt, uint32_t timeout)
{
    DWORD event_mask;
    DWORD dwrxcnt;
    COMMTIMEOUTS timeouts = {0};
    DCB dcbSerialParam = {0};
    //float tmp;

    //if(GetCommState(handle, &dcbSerialParam) == FALSE)
    //    return -1;

    //tmp = ((1.0f/(float)dcbSerialParam.BaudRate)*1000.0f*1.0f+0.5f);
    //if(tmp < 1.0f) tmp = 1.0f;
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = (DWORD)timeout;
    //tmp = ((1.0f/(float)dcbSerialParam.BaudRate)*1000.0f*11.0f+0.5f);
    //if(tmp < 1.0f) tmp = 1.0f;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if(SetCommTimeouts(handle, &timeouts) == FALSE)
        return -1;

/*
    if(WaitCommEvent(handle, &event_mask, NULL) == FALSE)
        return -1;
*/

    if(ReadFile(handle, buf, size, &dwrxcnt, NULL) == FALSE)
        return -1;

    *rxcnt = (uint32_t)dwrxcnt;
    
    return 0;
}

int32_t com_close(com_handle_t handle)
{
    if(CloseHandle(handle) == FALSE)
        return -1;

    return 0;
}

/******************************** END OF FILE *********************************/
