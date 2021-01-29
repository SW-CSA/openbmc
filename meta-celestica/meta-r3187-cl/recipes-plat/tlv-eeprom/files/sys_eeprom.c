/*
 * TLV EEPROM utility and based on ONIE platform
 # Copyright ONIE
 * Copyright 2018-present Celestica. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */



#include "onie_tlvinfo.h"
#include <getopt.h>

static u_int8_t eeprom[SYS_EEPROM_SIZE];
/*
 *  This macro defines the onie-syseeprom command line command.
 */
//usage:#define onie_syseeprom_trivial_usage
//usage:       "[[-le] | [-g <code>] | [-s <code>=<value>,...]]"
//usage:#define onie_syseeprom_full_usage "\n\n"
//usage:       "Display and program the system EEPROM data block.\n"
//usage:       "   With no arguments display the EEPROM contents.\n"
//usage:     "\n	-l	List the understood TLV codes and names."
//usage:     "\n	-e	Reset the EEPROM data."
//usage:     "\n	-g	Look up a TLV by code and write the value to stdout."
//usage:     "\n	-s	Set a TLV code to a value."
//usage:     "\n		If no value, TLV is deleted."
//usage:       "\n"

cmd_usage()
{
    static const char *usage =
	"Display and program the system EEPROM data block.\n"
	"Usage: onie-syseeprom [-h][-l] [-e] [-s <code>=<value>,...]\n"
	"   With no arguments display the EEPROM contents.\n"
	"   -h --help\n"
	"      Display usage\n"
	"   -l --list\n"
	"      List the understood TLV codes and names.\n"
	"   -e --erase\n"
	"      Reset the EEPROM data.\n"
	"   -g --get <code>\n"
	"      Look up a TLV by code and write the value to stdout.\n"
	"   -s --set <code>=<value>,<code>=<value>...\n"
	"      Set a TLV code to a value.\n"
	"      If no value, TLV is deleted.\n";

    fprintf(stderr, "%s", usage);
    exit(1);
}

int main(int argc, char **argv)
{
    int count = 0;
    int err = 0;
    int update = 0;
    char *value, *subopts, *tname;
    int index, c, i, option_index, tcode;
    char tlv_value[TLV_DECODE_VALUE_MAX_LEN];

    const size_t tlv_code_count = sizeof(tlv_code_list) /
	sizeof(tlv_code_list[0]);

    char *tokens[(tlv_code_count*2) + 1];
    const char *short_options = "hels:g:";
    const struct option long_options[] = {
	{"help",    no_argument,          0,    'h'},
	{"list",    no_argument,          0,    'l'},
	{"erase",   no_argument,          0,    'e'},
	{"set",     required_argument,    0,    's'},
	{"get",     required_argument,    0,    'g'},
	{0,         0,                    0,      0},
    };

    for (i = 0; i < tlv_code_count; i++) {
	    tokens[i*2] = malloc(6);
	    snprintf(tokens[(i*2)], 6,     "0x%x", tlv_code_list[i].m_code);
	    /* Allow for uppercase hex digits as well */
	    tokens[(i*2) + 1] = malloc(6);
	    snprintf(tokens[(i*2) + 1], 6, "0x%X", tlv_code_list[i].m_code);
    }
    tokens[tlv_code_count*2] = NULL;

    while (TRUE) {
	c = getopt_long(argc, argv, short_options,
			long_options, &option_index);
	if (c == EOF)
	    break;

	count++;
	switch (c) {
	case 'h':
	    cmd_usage();
	    break;

	case 'l':
	    show_tlv_code_list();
	    break;

	case 'e':
	    if (read_eeprom(eeprom)) {
                err = 1;
		goto syseeprom_err;
	    }
	    update_eeprom_header(eeprom);
	    update = 1;
	    break;

	case 's':
	    subopts = optarg;
	    while (*subopts != '\0' && !err) {
		if ((index = getsubopt(&subopts, tokens, &value)) != -1) {
		    if (read_eeprom(eeprom)) {
                        err = 1;
			goto syseeprom_err;
		    }
		    tcode = strtoul(tokens[index], NULL, 0);
		    for (i = 0; i < tlv_code_count; i++) {
			if (tlv_code_list[i].m_code == tcode) {
			    tname = tlv_code_list[i].m_name;
			}
		    }
		    if (tlvinfo_delete_tlv(eeprom, tcode) == TRUE) {
			    printf("Deleting TLV 0x%x: %s\n", tcode, tname);
		    }
		    if (value) {
			if (!tlvinfo_add_tlv(eeprom, tcode, value)) {
                            err = 1;
			    goto syseeprom_err;
			} else {
			    printf("Adding   TLV 0x%x: %s\n", tcode, tname);
			}
		    }
		    update = 1;
		} else {
		    err = 1;
		    printf("ERROR: Invalid option: %s\n", value);
		    goto syseeprom_err;
		}
	    }
	break;

	case 'g':
            if (read_eeprom(eeprom)) {
                err = 1;
                goto syseeprom_err;
            }
            tcode = strtoul(optarg, NULL, 0);
            if (tlvinfo_decode_tlv(eeprom, tcode, tlv_value)) {
                printf("%s\n", tlv_value);
            } else {
                err = 1;
                printf("ERROR: TLV code not present in EEPROM: 0x%02x\n", tcode);
            }
            goto syseeprom_err;
	break;

	default:
	    cmd_usage();
            err = 1;
	    break;
	}
    }
    if (!count) {
	if (argc > 1) {
	    cmd_usage();
            err = 1;
	} else {
	    if (read_eeprom(eeprom)) {
                err = 1;
                goto syseeprom_err;
            }
	    show_eeprom(eeprom);
	}
    }
    if (update) {
	if (prog_eeprom(eeprom)) {
            err = 1;
            goto syseeprom_err;
        }
	show_eeprom(eeprom);
    }
syseeprom_err:
    for (i = 0; i < tlv_code_count; i++) {
	free(tokens[i*2]);
	free(tokens[(i*2) + 1]);
    }
    return  (err == 0) ? 0 : 1;
}
