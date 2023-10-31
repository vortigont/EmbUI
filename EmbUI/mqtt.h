/*
    This file is part of EmbUI project
    https://github.com/vortigont/EmbUI

    Copyright © 2023 Emil Muratov (Vortigont)   https://github.com/vortigont/

    EmbUI is free software: you can redistribute it and/or modify
    it under the terms of MIT License https://opensource.org/license/mit/
*/

#pragma once
#include "EmbUI.h"

class FrameSendMQTT: public FrameSend {
    private:
        EmbUI *_eu;
    public:
        FrameSendMQTT(EmbUI *emb) : _eu(emb){}
        ~FrameSendMQTT() { _eu = nullptr; }
        bool available() const override { return _eu->mqttAvailable(); }
        void send(const String &data) override { };
        void send(const JsonObject& data) override;
};

