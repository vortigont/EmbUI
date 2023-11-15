#pragma once

void create_parameters();
void block_menu(Interface *interf, const JsonObject *data, const char* action);
void block_demopage(Interface *interf, const JsonObject *data, const char* action);
void action_demopage(Interface *interf, const JsonObject *data, const char* action);
void action_blink(Interface *interf, const JsonObject *data, const char* action);
