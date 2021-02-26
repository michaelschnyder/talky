#ifndef __STARTUP_H__
#define __STARTUP_H__

#include "FunctionFsm.h"
#include <Log4Esp.h>

#include "../UserInterface.h"
#include "DeviceConfig.h"
#include "Settings.h"
#include "../inbox/Contacts.h"
#include "../inbox/Inbox.h"
#include <AzureIoTMqttClient.h>
#include "pins.h"

class Startup {
    
    #define ONCOMPLETED_CALLBACK_SIGNATURE std::function<void(long)> 

    log4Esp::Logger logger = log4Esp::Logger("Startup");
    ONCOMPLETED_CALLBACK_SIGNATURE onCompletedCallback;

    FunctionState state_post;
    void post();

    FunctionState state_halt;
    void whileHalt();

    FunctionState state_checkSPIFFS;
    void checkSPIFFS();

    FunctionState state_loadConfig;
    void loadConfig();

    FunctionState state_checkSDCardFS;
    void checkSDCardFS();

    FunctionState state_loadSettings;
    void loadSettings();

    FunctionState state_loadContacts;
    void loadContacts();

    FunctionState state_startWifi;
    void startWifi();
    void waitForWifi();

    FunctionState state_connectToMqtt;
    void connectToMqtt();

    FunctionState state_startInbox;
    void startInbox();

    FunctionState state_ready;
    void whenReady();

    FunctionFsm fsm;

    UserInterface* ui;
    DeviceConfig* config;
    Settings* settings;
    Contacts* contacts;
    Inbox* inbox;
    AzureIoTMqttClient* client;

    enum Event {
        Continue,
        Halt,
    };

public:
    Startup(UserInterface* userInterface, DeviceConfig* config, Settings* settings, Contacts* contacts, Inbox* inbox, AzureIoTMqttClient* azClient) :
        state_post(nullptr,                            [this]() { post(); },            nullptr),
        state_halt(nullptr,                            [this]() { whileHalt(); },       nullptr),

        state_checkSPIFFS(nullptr,                     [this]() { checkSPIFFS(); },     nullptr),
        state_loadConfig(nullptr,                      [this]() { loadConfig(); },      nullptr),
        state_checkSDCardFS(nullptr,                   [this]() { checkSDCardFS(); },   nullptr),
        state_loadSettings(nullptr,                    [this]() { loadSettings(); },    nullptr),
        state_loadContacts(nullptr,                    [this]() { loadContacts(); },    nullptr),
        state_startWifi([this]() { startWifi(); },     [this]() { waitForWifi(); },     nullptr),
        state_connectToMqtt(nullptr,                   [this]() { connectToMqtt(); },   nullptr),
        state_startInbox(nullptr,                      [this]() { startInbox(); },      nullptr),

        state_ready([this]() { whenReady(); },          nullptr,                        nullptr),

        fsm(&state_post)
    {
        fsm.add_transition(&state_post, &state_checkSPIFFS, Event::Continue, nullptr);
        fsm.add_transition(&state_post, &state_halt, Event::Halt, nullptr);
        fsm.add_transition(&state_checkSPIFFS, &state_loadConfig, Event::Continue, nullptr);
        fsm.add_transition(&state_loadConfig, &state_checkSDCardFS, Event::Continue, nullptr);
        fsm.add_transition(&state_checkSDCardFS, &state_loadSettings, Event::Continue, nullptr);
        fsm.add_transition(&state_loadSettings, &state_loadContacts, Event::Continue, nullptr);
        fsm.add_transition(&state_loadContacts, &state_startWifi, Event::Continue, nullptr);
        fsm.add_transition(&state_startWifi, &state_connectToMqtt, Event::Continue, nullptr);
        fsm.add_transition(&state_connectToMqtt, &state_startInbox, Event::Continue, nullptr);
        
        fsm.add_transition(&state_startInbox, &state_ready, Event::Continue, nullptr);

        this->ui = userInterface;
        this->config = config;
        this->settings = settings;
        this->contacts = contacts;
        this->inbox = inbox;
        this->client = azClient;
    }

    void run();
    void onCompleted(ONCOMPLETED_CALLBACK_SIGNATURE callback);
};


#endif // __STARTUP_H__