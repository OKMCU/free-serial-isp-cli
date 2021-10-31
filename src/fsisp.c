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
 * 2021-10-12   Wentao SUN   first version
 * 
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <getopt.h>
#include "inc/serial.h"
#include "inc/crc.h"
/* Private define ------------------------------------------------------------*/
#define DEVICE_ADDR                         0xAA

/* Maximum number of bytes in payload of a packet */
#define PKT_PLD_SIZE                        128
/* Packet type enum */
#define TYPE_SET                            0x01
#define TYPE_GET                            0x02
#define TYPE_SUCCESS                        0x80
#define TYPE_FAILURE_UNKNOWN_REG            (0x80|0x00)
#define TYPE_FAILURE_ERR_LENGTH             (0x80|0x01)
#define TYPE_FAILURE_NOT_SUPPORT            (0x80|0x02)
#define TYPE_FAILURE_ERR_PASSWD             (0x80|0x03)
#define TYPE_FAILURE_ERR_SIGNATURE          (0x80|0x04)
#define TYPE_FAILURE_ERR_HAL                (0x80|0x05)
#define TYPE_FAILURE_ERR_PARAM              (0x80|0x06)
/* Private typedef -----------------------------------------------------------*/
typedef struct fsisp_opt_s {
    int version;
    int help;
    char *port;
    char *baudrate;
} fsisp_opt_t;

/**
 * First 4-byte header format of a packet.
 */
typedef struct packet_header_s {
    /* device address, ranging from 0x01 ~ 0xFF, 0x00 is for broadcasting */
    uint8_t dev_addr;
    /* packet type */
    uint8_t type;
    /* register address */
    uint8_t reg_addr;
    /** 
     * number of bytes in payload of a SET type packet, or
     * number of bytes to read of a GET type packet
     */
    uint8_t length;
} packet_header_t;

/**
 * Full packet format.
 */
typedef struct packet_s {
    /* packet header */
    packet_header_t header;
    /* packet payload */
    uint8_t payload[PKT_PLD_SIZE];
} packet_t;

typedef struct dev_attr_s {
    char part_number[128];
} dev_attr_t;
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
int parse_options(int argc, char **argv, fsisp_opt_t *opt)
{
    int c, prev_optind, option_index;
    static const struct option opts[] = {
        {"version",     no_argument,        NULL,   'v'},
        {"help",        no_argument,        NULL,   'h'},
        {"port",        required_argument,  NULL,   'p'},
        {"baudrate",    required_argument,  NULL,   'b'},
        {0,             0,                  0,       0 },
    };

    char **arg[sizeof(opts)/sizeof(struct option)] = {
        NULL,
        NULL,
        &opt->port,
        &opt->baudrate,
    };
#if 0
    for(int i = 0; i < argc; i++)
    {
        printf("argv[%d] = %s\r\n", i, argv[i]);
    }
#endif
    memset(opt, 0, sizeof(fsisp_opt_t));

    // reset optind to 1 to start scanning the elements in argv from beginning
    optind = 1;
    while(1)
    {
        prev_optind = optind;
        c = getopt_long(argc, argv, "vhp:b:", opts, &option_index);
        if(c == -1) break;
        else if(c == 0)
        {
            if(opts[option_index].has_arg == required_argument)
            {
                *arg[option_index] = optarg;
            }
        }
        else
        {
            switch(c)
            {
                case 'v':
                    opt->version = 1;
                    break;
                case 'h':
                    opt->help = 1;
                    break;
                case 'p':
                    opt->port= optarg;
                    break;
                case 'b':
                    opt->baudrate = optarg;
                    break;
                case '?':
                    return -1;
                default:
                    return -1;
            }
        }
    }

    return 0;
}

/**
  * @brief  send a packet to host side
  * @param  hdl [I] - serial port handle
  * @param  type [I] - packet type
  * @param  reg_addr [I] - register address
  * @param  length [I] - number of bytes in payload
  * @param  payload [I] - points to the payload data
  * @retval 0 = success, -1 = failure
  */
static int32_t send_packet(com_handle_t hdl, uint8_t type, uint8_t reg_addr, uint8_t length, const uint8_t *payload)
{
    uint8_t crc;
    packet_header_t pkt_header;
    if(length > PKT_PLD_SIZE) return -1;

    pkt_header.dev_addr = DEVICE_ADDR;
    pkt_header.type = type;
    pkt_header.reg_addr = reg_addr;
    pkt_header.length = length;

    crc = crc8_maxim((uint8_t *)&pkt_header, sizeof(pkt_header));
    if(type == TYPE_SET && length > 0)
        crc = crc8_maxim_update(crc, payload, length);

    com_send(hdl, (uint8_t *)&pkt_header, sizeof(pkt_header));
    if(length > 0)
        com_send(hdl, (uint8_t *)payload, length);
    com_send(hdl, (uint8_t *)&crc, 1);

    return 0;
}

/* Exported variables --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

int main(int argc, char **argv)
{
    fsisp_opt_t fsisp_opt;
    int32_t err;
    int32_t baudrate;
    /* buffer to receive uart data */
    uint8_t rxbuf[256];
    size_t rxlen;
    packet_t *pkt = (packet_t *)rxbuf;
    com_param_t com_param;
    com_handle_t com_handle;

    if(err = parse_options(argc, argv, &fsisp_opt))
        return err;

    if(fsisp_opt.version)
        printf("Free Serial ISP command line tool v1.0.0.\r\n");
        
    if(fsisp_opt.help)
        printf("This is help message.\r\n");

    com_param.baudrate = 115200;
    com_param.bytesize = COM_BYTESZ_8;
    com_param.parity = COM_PARITY_NONE;
    com_param.stopbits = COM_STOPBITS_1;

    if(fsisp_opt.baudrate)
    {
        baudrate = atoi(fsisp_opt.baudrate);
        if(baudrate > 0)
            com_param.baudrate = (uint32_t)baudrate;
    }
    
    printf("Openning serial port \"%s\", baudrate = %d...", fsisp_opt.port == NULL ? "<invalid>" : fsisp_opt.port, com_param.baudrate);
    if(com_open(fsisp_opt.port, &com_param, &com_handle))
    {
        printf("failed");
        return -1;
    }
    else
    {
        printf("OK");
    }
    printf("\r\n");

    printf("Connecting to device...");
    
    while(1)
    {
        if(send_packet(com_handle, TYPE_GET, 0x00, 0x80, NULL))
            return -1;
        if(com_recv(com_handle, rxbuf, sizeof(rxbuf), &rxlen, 50))
            return -1;

    #if 0
        for(size_t i = 0; i < rxlen; i++)
        {
            printf("0x%.2x ", rxbuf[i]);
        }
    #endif
        /* check packet CRC */
        if(rxlen > sizeof(packet_header_t) && rxbuf[rxlen-1] == crc8_maxim(rxbuf, rxlen-1))
        {
            rxbuf[rxlen-1] = 0;
            printf("OK");
            printf("\r\n");
            printf("MCU part number: %s\r\n", pkt->payload);
            break;
        }
    }

    printf("Closing to serial port...");
    if(com_close(com_handle))
    {
        printf("failed");
        return -1;
    }
    else
    {
        printf("OK");
    }
    printf("\r\n");

    return 0;
}

/******************************** END OF FILE *********************************/
