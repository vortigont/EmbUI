#pragma once

void create_parameters();
void block_menu(Interface *interf, JsonObject *data, const char* action);
void block_demopage(Interface *interf, JsonObject *data, const char* action);
void action_demopage(Interface *interf, JsonObject *data, const char* action);
void action_blink(Interface *interf, JsonObject *data, const char* action);
void setRate(Interface *interf, JsonObject *data, const char* action);
void sensorPublisher();
