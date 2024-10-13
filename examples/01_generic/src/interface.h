#pragma once

void create_parameters();
void block_menu(Interface *interf, JsonObjectConst data, const char* action);
void block_demopage(Interface *interf, JsonObjectConst data, const char* action);
void action_demopage(Interface *interf, JsonObjectConst data, const char* action);
void action_blink(Interface *interf, JsonObjectConst data, const char* action);
