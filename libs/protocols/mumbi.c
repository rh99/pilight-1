/*
	Copyright (C) 2014 CurlyMo

	This file is part of pilight.

    pilight is free software: you can redistribute it and/or modify it under the 
	terms of the GNU General Public License as published by the Free Software 
	Foundation, either version 3 of the License, or (at your option) any later 
	version.

    pilight is distributed in the hope that it will be useful, but WITHOUT ANY 
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR 
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../pilight.h"
#include "common.h"
#include "log.h"
#include "protocol.h"
#include "hardware.h"
#include "binary.h"
#include "gc.h"
#include "mumbi.h"

void mumbiCreateMessage(int systemcode, int unitcode, int state) {
	mumbi->message = json_mkobject();
	json_append_member(mumbi->message, "systemcode", json_mknumber(systemcode));
	json_append_member(mumbi->message, "unitcode", json_mknumber(unitcode));
	if(state == 0) {
		json_append_member(mumbi->message, "state", json_mkstring("on"));
	} else {
		json_append_member(mumbi->message, "state", json_mkstring("off"));
	}
}

void mumbiParseBinary(void) {
	int systemcode = binToDec(mumbi->binary, 0, 4);
	int unitcode = binToDec(mumbi->binary, 5, 9);
	int state = mumbi->binary[11];
	if(unitcode > 0) {
		mumbiCreateMessage(systemcode, unitcode, state);
	}
}

void mumbiCreateLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		mumbi->raw[i]=(mumbi->plslen->length);
		mumbi->raw[i+1]=(mumbi->pulse*mumbi->plslen->length);
		mumbi->raw[i+2]=(mumbi->pulse*mumbi->plslen->length);
		mumbi->raw[i+3]=(mumbi->plslen->length);
	}
}

void mumbiCreateHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		mumbi->raw[i]=(mumbi->plslen->length);
		mumbi->raw[i+1]=(mumbi->pulse*mumbi->plslen->length);
		mumbi->raw[i+2]=(mumbi->plslen->length);
		mumbi->raw[i+3]=(mumbi->pulse*mumbi->plslen->length);
	}
}
void mumbiClearCode(void) {
	mumbiCreateLow(0,47);
}

void mumbiCreateSystemCode(int systemcode) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(systemcode, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			mumbiCreateHigh(x, x+3);
		}
	}
}

void mumbiCreateUnitCode(int unitcode) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unitcode, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			mumbiCreateHigh(20+x, 20+x+3);
		}
	}
}

void mumbiCreateState(int state) {
	if(state == 0) {
		mumbiCreateHigh(44, 47);
	}
}

void mumbiCreateFooter(void) {
	mumbi->raw[48]=(mumbi->plslen->length);
	mumbi->raw[49]=(PULSE_DIV*mumbi->plslen->length);
}

int mumbiCreateCode(JsonNode *code) {
	int systemcode = -1;
	int unitcode = -1;
	int state = -1;
	int tmp;

	json_find_number(code, "systemcode", &systemcode);
	json_find_number(code, "unitcode", &unitcode);
	if(json_find_number(code, "off", &tmp) == 0)
		state=1;
	else if(json_find_number(code, "on", &tmp) == 0)
		state=0;

	if(systemcode == -1 || unitcode == -1 || state == -1) {
		logprintf(LOG_ERR, "mumbi: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(systemcode > 31 || systemcode < 0) {
		logprintf(LOG_ERR, "mumbi: invalid systemcode range");
		return EXIT_FAILURE;
	} else if(unitcode > 31 || unitcode < 0) {
		logprintf(LOG_ERR, "mumbi: invalid unitcode range");
		return EXIT_FAILURE;
	} else {
		mumbiCreateMessage(systemcode, unitcode, state);
		mumbiClearCode();
		mumbiCreateSystemCode(systemcode);
		mumbiCreateUnitCode(unitcode);
		mumbiCreateState(state);
		mumbiCreateFooter();
	}
	return EXIT_SUCCESS;
}

void mumbiPrintHelp(void) {
	printf("\t -s --systemcode=systemcode\tcontrol a device with this systemcode\n");
	printf("\t -u --unitcode=unitcode\t\tcontrol a device with this unitcode\n");
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
}

void mumbiInit(void) {

	protocol_register(&mumbi);
	protocol_set_id(mumbi, "mumbi");
	protocol_device_add(mumbi, "mumbi", "Mumbi Switches");
	protocol_plslen_add(mumbi, 312);
	mumbi->devtype = SWITCH;
	mumbi->hwtype = RF433;
	mumbi->pulse = 3;
	mumbi->rawlen = 50;
	mumbi->binlen = 12;
	mumbi->lsb = 3;

	options_add(&mumbi->options, 's', "systemcode", has_value, config_id, "^(3[012]?|[012][0-9]|[0-9]{1})$");
	options_add(&mumbi->options, 'u', "unitcode", has_value, config_id, "^(3[012]?|[012][0-9]|[0-9]{1})$");
	options_add(&mumbi->options, 't', "on", no_value, config_state, NULL);
	options_add(&mumbi->options, 'f', "off", no_value, config_state, NULL);

	protocol_setting_add_string(mumbi, "states", "on,off");	
	protocol_setting_add_number(mumbi, "readonly", 0);
	
	mumbi->parseBinary=&mumbiParseBinary;
	mumbi->createCode=&mumbiCreateCode;
	mumbi->printHelp=&mumbiPrintHelp;
}