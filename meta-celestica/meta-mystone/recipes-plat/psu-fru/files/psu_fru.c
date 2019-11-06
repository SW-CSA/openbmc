#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psu_fru.h"

unsigned char buff[PSU_EEPROM_LENGTH];

dps1100fb dps1100_info = {
    .mfr_id = {
        .offset = MFR_ID,
        .length = MFR_ID_Length,
    },
    .product_name = { 
        .offset = Product_Name,
        .length = Product_Name_Length,
    },
    .serial_num = { 
        .offset = Serial_Number,
        .length = Serial_Number_Length,
    },
    .product_ver = { 
        .offset = Product_Version,
        .length = Product_Version_Length,
    },
};

void usage(void)
{
    printf("psu_fru <eeprom_path>\n");
}

int main(int argc, char *argv[])
{
    FILE *fp;
    if(argc < 2)
    {
        usage();
    }

    fp = fopen(argv[1],"rb");
    if(fp == NULL)
    {
        printf("Open file [%s] error!\n", argv[1]);
        return 1;
    }
    if(dps1100_info_init())
    {
        dps1100_info_deinit();
        printf("Initializing error!\n");
        return 1;
    }
    fread(buff, sizeof(unsigned char), PSU_EEPROM_LENGTH, fp);
    
    get_data(buff, &dps1100_info.mfr_id);
    get_data(buff, &dps1100_info.product_name);
    get_data(buff, &dps1100_info.serial_num);
    get_data(buff, &dps1100_info.product_ver);
    
    printf("MFR ID: %s\nProduct Name: %s\nSerial Number: %s\nProduct Version: %s\n",
            dps1100_info.mfr_id.data, dps1100_info.product_name.data,
            dps1100_info.serial_num.data, dps1100_info.product_ver.data);

    dps1100_info_deinit();
    fclose(fp);
    
    return 0;
}

int get_data(char *raw, psu_data *data)
{
    memcpy(data->data,raw+data->offset,data->length);
    return 0;
}

int dps1100_info_deinit(void)
{
    free(dps1100_info.mfr_id.data);
    free(dps1100_info.product_name.data);
    free(dps1100_info.serial_num.data);
    free(dps1100_info.product_ver.data);
}

int dps1100_info_init(void)
{
    dps1100_info.mfr_id.data = (unsigned char *)malloc(dps1100_info.mfr_id.length + 1);
    if(dps1100_info.mfr_id.data == NULL)
    {
        return -1;
    }
    dps1100_info.product_name.data = (unsigned char *)malloc(dps1100_info.product_name.length + 1);
    if(dps1100_info.mfr_id.data == NULL)
    {
        return -1;
    }
    dps1100_info.serial_num.data = (unsigned char *)malloc(dps1100_info.serial_num.length + 1);
    if(dps1100_info.mfr_id.data == NULL)
    {
        return -1;
    }
    dps1100_info.product_ver.data = (unsigned char *)malloc(dps1100_info.product_ver.length  + 1);
    if(dps1100_info.mfr_id.data == NULL)
    {
        return -1;
    }
    
    memset(dps1100_info.mfr_id.data, 0, dps1100_info.mfr_id.length + 1);
    memset(dps1100_info.product_name.data, 0, dps1100_info.product_name.length + 1);
    memset(dps1100_info.serial_num.data, 0, dps1100_info.serial_num.length + 1);
    memset(dps1100_info.product_ver.data, 0, dps1100_info.product_ver.length + 1);
    memset(buff, 0, sizeof(buff));

    return 0;
}

