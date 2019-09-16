#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eeprom_data.h"
#include "ezxml.h"

#define CONFIGURATION_PATH "/etc/eeprom-data/eeprom.xml"

static void usage(void)
{
    printf("eeprom_data -d <");
    ezxml_t f1 = ezxml_parse_file(CONFIGURATION_PATH), device;
    int i = 0;
    for (device = ezxml_child(f1, "Device"); device; device = device->next) {
        if(i) printf(" | ");
        i++;
        printf("%s", ezxml_attr(device, "Name"));
    }
    printf("> -c <config file path>\n");
}

static int free_eeprom_data(eeprom_data *eeprom)
{
    eeprom_data *p_eeprom;
    p_eeprom = eeprom;
    // for(p_eeprom = eeprom; p_eeprom->data; p_eeprom = p_eeprom->next)
    {
        eeprom_data_item *p_eeprom_data_item;
        for(p_eeprom_data_item = p_eeprom->data; p_eeprom_data_item;)
        {
            eeprom_data_item *temp;
            temp = p_eeprom_data_item;
            p_eeprom_data_item = p_eeprom_data_item->next;
            free(temp->name);
            free(temp->data);
            free(temp);
        }

        free(p_eeprom->data);
        free(p_eeprom->path);
    }
    return 0;
}

static int get_eeprom_data_format(eeprom_data *eeprom, const char *dev_name, const char *config_path)
{
    ezxml_t f1 = ezxml_parse_file(config_path), device, dataitem;
    eeprom_data *p_eeprom;

    p_eeprom = eeprom;
    p_eeprom->data = malloc(sizeof(eeprom_data_item));
    memset(p_eeprom->data, 0, sizeof(eeprom_data_item));
    // printf("eeprom->data: 0x%x\n",p_eeprom->data);
    eeprom_data_item *p_eeprom_data_item;
    p_eeprom_data_item = p_eeprom->data;
    p_eeprom_data_item->next = NULL;
    if(!p_eeprom->data) return -1;
    for (device = ezxml_child(f1, "Device"); device; device = device->next) {
        if(strcmp(ezxml_attr(device, "Name"), dev_name)) continue;
        int len = strlen(ezxml_attr(device, "Path")) + 1;
        p_eeprom->path = malloc(len);
        if(p_eeprom->path == NULL) return -1;
        memset(p_eeprom->path, 0, len);
        if(!strcmp(ezxml_attr(device, "SizeSystem"), "Hex")) {
            sscanf(ezxml_attr(device, "Size"), "%x", &p_eeprom->size);
        }
        else {
            sscanf(ezxml_attr(device, "Size"), "%d", &p_eeprom->size);
        }
        strcpy(p_eeprom->path, ezxml_attr(device, "Path"));
        for (dataitem = ezxml_child(device, "DataItem"); dataitem; ) {
            len = strlen(ezxml_attr(dataitem, "Name")) + 1;
            p_eeprom_data_item->name = malloc(len);
            if(p_eeprom_data_item->name == NULL) return -1;
            memset(p_eeprom_data_item->name, 0, len);
            strcpy(p_eeprom_data_item->name, ezxml_attr(dataitem, "Name"));
            if(!strcmp(ezxml_attr(dataitem, "OffsetSystem"), "Hex")) {
                sscanf(ezxml_attr(dataitem, "Offset"), "%x", &p_eeprom_data_item->offset);
            }
            else {
                sscanf(ezxml_attr(dataitem, "Offset"), "%d", &p_eeprom_data_item->offset);
            }
            if(!strcmp(ezxml_attr(dataitem, "LengthSystem"), "Hex")) {
                sscanf(ezxml_attr(dataitem, "Length"), "%x", &p_eeprom_data_item->length);
            }
            else {
                sscanf(ezxml_attr(dataitem, "Length"), "%d", &p_eeprom_data_item->length);
            }
            p_eeprom_data_item->data = malloc(p_eeprom_data_item->length + 1);
            if(p_eeprom_data_item->data == NULL) return -1;
            memset(p_eeprom_data_item->data, 0, p_eeprom_data_item->length + 1);
            dataitem = dataitem->next;
            // printf("%s: Offset: %d Length: %d\n",p_eeprom->data->name, p_eeprom->data->offset, p_eeprom->data->length);
            if(dataitem) {
                p_eeprom_data_item->next = malloc(sizeof(eeprom_data_item));
                if(!p_eeprom_data_item->next) return -1;
                p_eeprom_data_item = p_eeprom_data_item->next;
                p_eeprom_data_item->next = NULL;
            }
        }
    }
    // p_eeprom = eeprom;
    // p_eeprom_data_item = p_eeprom->data;
    // printf("Device:%s\tPath:%s\tSzie:%d\n",dev_name, p_eeprom->path, p_eeprom->size);
    // for(; p_eeprom_data_item; p_eeprom_data_item = p_eeprom_data_item->next)
    // {
    //     printf("%s: Offset: 0x%x Length: %d\n",p_eeprom_data_item->name, p_eeprom_data_item->offset, p_eeprom_data_item->length);
    // }
    ezxml_free(f1);
    if(p_eeprom->path == NULL) return -1;
    return 0;
}

static int get_eeprom_data(eeprom_data *eeprom)
{
    FILE *fp_eeprom;
    fp_eeprom = fopen(eeprom->path,"rb");
    if(fp_eeprom == NULL)
    {
        printf("Open file [%s] error!\n", eeprom->path);
        return -1;
    }

    unsigned char *eeprom_buff;
    eeprom_buff = malloc(eeprom->size);
    if(!eeprom_buff) return -1;
    memset(eeprom_buff, 0, eeprom->size);
    fread(eeprom_buff, sizeof(unsigned char), eeprom->size, fp_eeprom);

    eeprom_data *p_eeprom;
    p_eeprom = eeprom;
    // for(p_eeprom = eeprom; p_eeprom->data; p_eeprom = p_eeprom->next)
    {
        // printf("Device:%s\tSzie:%d\n", p_eeprom->path, p_eeprom->size);
        eeprom_data_item *p_eeprom_data_item;
        for(p_eeprom_data_item = p_eeprom->data; p_eeprom_data_item; p_eeprom_data_item = p_eeprom_data_item->next)
        {
            memcpy(p_eeprom_data_item->data, eeprom_buff + p_eeprom_data_item->offset, p_eeprom_data_item->length);
            printf("%s: %s\n",p_eeprom_data_item->name, p_eeprom_data_item->data);
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        usage();
        exit(0);
    }

    char *config_path;
    char *dev;
    int i;
    config_path = NULL;
    dev = NULL;
    for(i = 1; i < argc; i++) {
        if(!strcasecmp(argv[i], "-c")) {
            config_path = argv[++i];
        } else if(!strcasecmp(argv[i], "-d")) {
            dev = argv[++i];
        }
    }

    if(!dev) {
        usage();
        exit(1);
    }
    if(!config_path) {
        config_path = CONFIGURATION_PATH;
    }

    eeprom_data *eeprom = malloc(sizeof(eeprom_data));
    eeprom->data = NULL;
    eeprom->path = NULL;
    // eeprom->next = NULL;
    
    if(!get_eeprom_data_format(eeprom, dev, config_path))
    {
        get_eeprom_data(eeprom);
    }
    free_eeprom_data(eeprom);

    exit(0);
}
