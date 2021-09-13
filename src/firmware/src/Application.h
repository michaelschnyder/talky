#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <Log4Esp.h>
#include <AzureIoTMqttClient.h>

#include "core/Startup.h"
#include "core/Shutdown.h"
#include "core/DeviceConfig.h"
#include "core/Settings.h"

#include "ui/UserInterface.h"

#include "inbox/Contacts.h"
#include "inbox/Inbox.h"
#include "http/FileUploader.h"

#include "audio/AudioRecorder.h"
#include "audio/AudioPlayer.h"

#include "FunctionFsm.h"

class Application
{
    log4Esp::Logger logger = log4Esp::Logger("Application");

    enum Event {
        SYSTEM_READY,
        SYSTEM_SHUTDOWN,
        BUTTON1_CLICK,
        BUTTON2_CLICK,
        BUTTON3_CLICK,
        BUTTON1_LONGSTART,
        BUTTON1_LONG_RELEASE,
        BUTTON2_LONGSTART,
        BUTTON2_LONG_RELEASE,
        BUTTON3_LONGSTART,
        BUTTON3_LONG_RELEASE,
        
        RECORDING_LENGTH_EXCEEDED,
        MESSAGE_FOUND,
        MESSAGE_NOTFOUND,
        MESSAGE_PLAYED,
        SEND_MESSAGE,
        DISCARD_MESSAGE,
        MESSAGE_SENT,
        DOWNLOAD_MESSAGE,
        MESSAGE_DOWNLOADED
    };

    DeviceConfig config;
    Settings settings;

    AzureIoTMqttClient client;
    UserInterface *ui;
    AudioRecorder *recorder;
    FileUploader* uploader;

    Contacts contacts;
    Inbox inbox;
    AudioPlayer *player;
    
    Startup startup;
    Shutdown shutdown;

    FunctionState state_startup;
    void whileStarting();
    void afterStarting();

    FunctionState state_shutdown;
    void whileShuttingDown();

    FunctionState state_idle;
    void beforeIdling();
    void whileIdling();

    FunctionState state_record1;
    FunctionState state_record2;
    FunctionState state_record3;
    void recordMessageFor(int buttonId);
    void whileMessageRecording();

    FunctionState state_validate;
    void validateRecording();

    FunctionState state_send;
    void sendLastMessage();
    void whileMessageSending();

    FunctionState state_tryPlay1;
    FunctionState state_tryPlay2;
    FunctionState state_tryPlay3;
    void tryPlayMessageFrom(int buttonId);

    FunctionState state_play;
    void whileMessagePlaying();
    void messagePlayingEnded();

    FunctionState state_receiveMessage;
    void whileReceivingMessage();

    FunctionFsm fsm;

    bool isAppRunning = false;

    void dispatchCloudCommand(String, JsonObject&);
    void showNewMessageFrom(Contact*);
    
public:
    Application(UserInterface*, AudioRecorder*, AudioPlayer*, FileUploader*);

    void setup();
    void run();
};

#endif // __APPLICATION_H__