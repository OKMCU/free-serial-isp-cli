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
#include <string.h>
#include <getopt.h>
/* Private define ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

int main(int argc, char **argv)
{
    const char *optstring = "vhcp:b:";
    int c;
    struct option opts[] = {
        {"version",     no_argument,        NULL,   'v'},
        {"help",        no_argument,        NULL,   'h'},
        {"connect",     no_argument,        NULL,   'c'},
        {"port",        required_argument,  NULL,   'p'},
        {"baudrate",    required_argument,  NULL,   'b'},
    };

    while((c = getopt_long(argc, argv, optstring, opts, NULL)) != -1)
    {
        switch(c)
        {
            case 'v':
                printf("Free Serial ISP command line tool v1.0.0.\r\n");
                break;
            case 'h':
                printf("This is help message.\r\n");
                break;
            case 'c':
                printf("Connecting to device...Please wait...\r\n");
                break;
            case 'p':
                printf("port = %s\r\n", optarg);
                break;
            case 'b':
                printf("baudrate = %s\r\n", optarg);
                break;
            case '?':
                printf("unkown option \"%s\"\r\n", argv[optind]);
                break;
            default:
                printf("should not reach here!\r\n");
                break;
        }
    }
}

/******************************** END OF FILE *********************************/
