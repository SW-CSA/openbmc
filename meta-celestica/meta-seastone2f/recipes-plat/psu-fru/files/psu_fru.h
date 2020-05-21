#ifndef _PSU_H_
#define _PSU_H_

#define PSU_EEPROM_LENGTH       0xA0

#define CUSTOMER_ORDER_NUMBER                       0x00
#define CUSTOMER_ORDER_NUMBER_LENGTH                0x20
#define POWER_SUPPLY_DESC                           0x20
#define POWER_SUPPLY_DESC_LENGTH                    0x20
#define POWER_SUPPLY_SERIAL_NUMBER                  0x40
#define POWER_SUPPLY_SERIAL_NUMBER_LENGTH           0x10
#define MANUFACTURER_PART_NUMBER_REVISION           0x50
#define MANUFACTURER_PART_NUMBER_REVISION_LENGTH    0x10
#define CUSTOMER_PART_NUMBER_REVISION               0x60
#define CUSTOMER_PART_NUMBER_REVISION_LENGTH        0x10
#define MANUFACTURING_DATA_CODE                     0x70
#define MANUFACTURING_DATA_CODE_LENGTH              0x10
#define MANUFACTURING_LOCATION                      0x80
#define MANUFACTURING_LOCATION_LENGTH               0x10
#define ROHS_COMPLIANCE                             0x90
#define ROHS_COMPLIANCE_LENGTH                      0x0F
#define CHECKSUM                                    0x9F
#define CHECKSUM_LENGTH                             0x01

#define MFR_ID                  0x2C
#define MFR_ID_Length           12
#define Product_Name            0x4D
#define Product_Name_Length     17
#define Serial_Number           0x38
#define Serial_Number_Length    14
#define Product_Version         0x46
#define Product_Version_Length    3

typedef struct _eeprom_psu_data {
    int offset;
    unsigned int length;
    unsigned char *data;
} psu_data, *p_psu_data;

struct psu_eeprom {
    psu_data customer_order_num;
    psu_data power_supply_desc;
    psu_data power_supply_sn;
    psu_data manufacturer_part_num_rev;
    psu_data customer_part_num_rev;
    psu_data manufacturing_data_code;
    psu_data manufacturing_location;
    psu_data rohs_compliance;
    psu_data checksum;
};

typedef struct _dps1100fb {
    psu_data mfr_id;
    psu_data product_name;
    psu_data serial_num;
    psu_data product_ver;
} dps1100fb, *p_dps1100fb;

int get_data(char *raw, psu_data *data);
int dps1100_info_deinit(void);
int dps1100_info_init(void);

#endif

