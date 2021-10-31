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

typedef struct dev_attr_mcu_s {
    char part_number[128];
    char uuid[128];
} dev_attr_mcu_t;

typedef struct dev_attr_bldr_s {
    uint8_t major_ver;
    uint8_t minor_ver;
    uint16_t build_ver;
    uint32_t addr;
    uint32_t size;
} dev_attr_bldr_t;

typedef struct dev_attr_s {
    dev_attr_mcu_t mcu;
    dev_attr_bldr_t bldr;
} dev_attr_t;
/* Private macro -------------------------------------------------------------*/
/**
 * break a 32-bit value into bytes
 */
#define BREAK_UINT32(var, ByteNum) \
          (uint8_t)((uint32_t)(((var) >>((ByteNum) * 8)) & 0x00FF))

/**
 * build a 32-bit value from bytes
 */
#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32_t)((uint32_t)((Byte0) & 0x00FF) \
          + ((uint32_t)((Byte1) & 0x00FF) << 8) \
          + ((uint32_t)((Byte2) & 0x00FF) << 16) \
          + ((uint32_t)((Byte3) & 0x00FF) << 24)))

/**
 * build a 16-bit value from bytes
 */
#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

/**
 * break a 16-bit value into bytes
 */
#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

/**
 * bitmask value of one bit
 */
#define BIT(n)      (1<<n)
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
int32_t send_packet(com_handle_t hdl, uint8_t dev_addr, uint8_t type, uint8_t reg_addr, uint8_t length, const uint8_t *payload)
{
    uint8_t crc;
    packet_header_t pkt_header;
    if(length > PKT_PLD_SIZE) return -1;

    pkt_header.dev_addr = dev_addr;
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

int32_t read_reg(com_handle_t hdl, uint8_t dev_addr, uint8_t reg_addr, uint8_t *pdata, uint8_t size, uint8_t *length, uint32_t timeout)
{
    uint8_t rxbuf[256];
    size_t rxlen;
    packet_t *pkt = (packet_t *)rxbuf;

    if(send_packet(hdl, dev_addr, TYPE_GET, reg_addr, size, NULL))
        return -1;
    if(com_recv(hdl, rxbuf, sizeof(rxbuf), &rxlen, timeout))
        return -1;

    if(rxlen <= sizeof(packet_header_t) || rxbuf[rxlen-1] != crc8_maxim(rxbuf, rxlen-1))
        return -1;

    if(pkt->header.dev_addr != dev_addr ||
       pkt->header.type != TYPE_SET     ||
       pkt->header.reg_addr != reg_addr ||
       pkt->header.length > size)
        return -1;

    if(length) *length = pkt->header.length;
    memcpy(pdata, pkt->payload, pkt->header.length);

    return 0;
}

int32_t get_dev_attr(com_handle_t hdl, dev_attr_t *attr)
{
    uint8_t payload[128];
    uint8_t len;

    /* MCU part number */
    if(read_reg(hdl, DEVICE_ADDR, 0x00, attr->mcu.part_number, sizeof(attr->mcu.part_number), &len, 100))
        return -1;

    /* MCU UUID */
    if(read_reg(hdl, DEVICE_ADDR, 0x01, attr->mcu.uuid, sizeof(attr->mcu.uuid), &len, 100))
        return -1;

    /* Bootloader version */
    if(read_reg(hdl, DEVICE_ADDR, 0x02, payload, 4, &len, 100))
        return -1;
    attr->bldr.major_ver = payload[0];
    attr->bldr.minor_ver = payload[1];
    attr->bldr.build_ver = BUILD_UINT16(payload[3], payload[2]);

    /* Bootloader flash address */
    if(read_reg(hdl, DEVICE_ADDR, 0x03, payload, 4, &len, 100))
        return -1;
    attr->bldr.addr = BUILD_UINT32(payload[3], payload[2], payload[1], payload[0]);

    /* Bootloader size */
    if(read_reg(hdl, DEVICE_ADDR, 0x04, payload, 4, &len, 100))
        return -1;
    attr->bldr.size = BUILD_UINT32(payload[3], payload[2], payload[1], payload[0]);

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
    com_param_t com_param;
    com_handle_t com_handle;
    dev_attr_t dev_attr;

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

    memset(&dev_attr, 0x00, sizeof(dev_attr));
    while(get_dev_attr(com_handle, &dev_attr) != 0);

    printf("OK");
    printf("\r\n");
    printf("MCU part number: %s\r\n", dev_attr.mcu.part_number);
    printf("MCU UUID: %s\r\n", dev_attr.mcu.uuid);
    printf("Bootloader version: v%d.%d.%d\r\n", dev_attr.bldr.major_ver, dev_attr.bldr.minor_ver, dev_attr.bldr.build_ver);
    printf("Bootloader flash adress: 0x%.8x\r\n", dev_attr.bldr.addr);
    printf("Bootloader size: %d\r\n", dev_attr.bldr.size);

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
