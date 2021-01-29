#ifndef _EEPROM_DATA_H_
#define _EEPROM_DATA_H_

typedef struct _eeprom_data_item {
    char *name;
    unsigned int offset;
    unsigned int length;
    char *data;
    struct _eeprom_data_item *next;
} eeprom_data_item;

typedef struct _eeprom_data {
    eeprom_data_item *data;
    char *path;
    unsigned int size;
    // struct _eeprom_data *next;
} eeprom_data;

#endif
