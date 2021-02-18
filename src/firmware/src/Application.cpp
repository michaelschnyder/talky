#include "Application.h"
#include "SD.h"
#include "WiFi.h"

#define MINIMAL_MESSAGE_LENGTH_IN_MS 1000
#define MAXIMAL_MESSAGE_LENGTH_IN_MS 24000

Application::Application(UserInterface* ui, AudioRecorder* recorder, AudioPlayer* player, FileUploader* uploader) : Application()
{
    this->ui = ui;
    this->recoder = recoder;
    this->player = player;
    this->uploader = uploader;

    this->fsm.add_transition(&state_startup, &state_idle, SYSTEM_READY, nullptr);

    this->fsm.add_transition(&state_idle, &state_record1, BUTTON1_LONGSTART, nullptr);
    this->fsm.add_transition(&state_idle, &state_record2, BUTTON2_LONGSTART, nullptr);
    this->fsm.add_transition(&state_idle, &state_record3, BUTTON3_LONGSTART, nullptr);
    this->fsm.add_transition(&state_record1, &state_validate, BUTTON1_LONG_RELEASE, nullptr);
    this->fsm.add_transition(&state_record2, &state_validate, BUTTON2_LONG_RELEASE, nullptr);
    this->fsm.add_transition(&state_record3, &state_validate, BUTTON3_LONG_RELEASE, nullptr);
    this->fsm.add_transition(&state_record1, &state_validate, RECORDING_LENGTH_EXCEEDED, nullptr);
    this->fsm.add_transition(&state_record2, &state_validate, RECORDING_LENGTH_EXCEEDED, nullptr);
    this->fsm.add_transition(&state_record3, &state_validate, RECORDING_LENGTH_EXCEEDED, nullptr);

    this->fsm.add_transition(&state_validate, &state_send, SEND_MESSAGE, nullptr);
    this->fsm.add_transition(&state_send, &state_idle, MESSAGE_SENT, nullptr);
    this->fsm.add_transition(&state_validate, &state_idle, DISCARD_MESSAGE, nullptr);

    this->fsm.add_transition(&state_idle, &state_play1, BUTTON1_CLICK, nullptr);
    this->fsm.add_transition(&state_idle, &state_play2, BUTTON2_CLICK, nullptr);
    this->fsm.add_transition(&state_idle, &state_play3, BUTTON3_CLICK, nullptr);
    this->fsm.add_transition(&state_play1, &state_idle, MESSAGE_PLAYED, nullptr);
    this->fsm.add_transition(&state_play2, &state_idle, MESSAGE_PLAYED, nullptr);
    this->fsm.add_transition(&state_play3, &state_idle, MESSAGE_PLAYED, nullptr);

    this->ui->onButtonEvent([this](ButtonEvent evt) {
    
        if (evt.action == Action::Clicked) {
            switch (evt.buttonId)
            {
            case 1: this->fsm.trigger(Event::BUTTON1_CLICK); break;
            case 2: this->fsm.trigger(Event::BUTTON2_CLICK); break;
            case 3: this->fsm.trigger(Event::BUTTON3_CLICK); break;
            default:
                break;
            }
        }

        if (evt.action == Action::LongPressStart) {
            switch (evt.buttonId)
            {
            case 1: this->fsm.trigger(Event::BUTTON1_LONGSTART); break;
            case 2: this->fsm.trigger(Event::BUTTON2_LONGSTART); break;
            case 3: this->fsm.trigger(Event::BUTTON3_LONGSTART); break;
            default:
                break;
            }
        }

        if (evt.action == Action::LongPressEnd) {
            switch (evt.buttonId)
            {
            case 1: this->fsm.trigger(Event::BUTTON1_LONG_RELEASE); break;
            case 2: this->fsm.trigger(Event::BUTTON2_LONG_RELEASE); break;
            case 3: this->fsm.trigger(Event::BUTTON3_LONG_RELEASE); break;
            default:
                break;
            }
        }
     });
}

void Application::run() {    
    fsm.run_machine();

    client.loop();
}

void Application::start() 
{
    config.load();

    settings.load();

    WiFi.mode(WIFI_STA);    // Station Mode, i.e. connect to a WIFI and don't serve as AP
    WiFi.persistent(false); // Do not store WIFI information in EEPROM.

    logger.trace(F("Connecting to WLAN with SSID '%s'. This may take some time..."), settings.getWifiSSID().c_str());

    WiFi.begin(settings.getWifiSSID().c_str(), settings.getWifiKey().c_str());
}

void Application::whileStarting() 
{
    this->ui->isBusy(true);

    if (WiFi.isConnected()) {
        logger.trace(F("WiFi connection established. IP address: %s, MAC: %s"), WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str());

        this->ui->showWelcome();
        this->fsm.trigger(Event::SYSTEM_READY);

        client.setup(config.getAzIoTHubName().c_str(), config.getDeviceId().c_str(), config.getAzIoTSASToken().c_str());
    }
}

void Application::whenIdle() 
{
    this->ui->isBusy(false);
}

char currentRecordingName[30];
long currentBytesSent = 0;
File f;

void Application::recordMessageFor(int buttonId) 
{
    sprintf(currentRecordingName, "/to_btn%i_at_%i.wav\0", buttonId, millis());

    logger.trace(F("Recording to %s"), currentRecordingName);

    f = SD.open(currentRecordingName, FILE_WRITE);
    this->recoder->record(&f);    
}


void Application::whileMessageRecording() 
{
    int recordingDuration = this->recoder->duration();

    if (recordingDuration >= MAXIMAL_MESSAGE_LENGTH_IN_MS) {
        logger.trace(F("Stopped recording because current recording lenght (%ims) is above maximum of %ims"), recordingDuration, MAXIMAL_MESSAGE_LENGTH_IN_MS);
        this->fsm.trigger(RECORDING_LENGTH_EXCEEDED);
    }

    this->ui->showRecordingProgress((recordingDuration / 1000) + 1);
}

void Application::validateRecording() 
{
    long lenghtInMs = this->recoder->stop();
    f.close();

    logger.trace(F("Stopped recording. Validating recording length: %ims"), lenghtInMs);

    if (lenghtInMs >= MINIMAL_MESSAGE_LENGTH_IN_MS) {
        this->fsm.trigger(Event::SEND_MESSAGE);
    }
    else {
        logger.trace(F("Message length was below threshold of %ims, ignoring."), MINIMAL_MESSAGE_LENGTH_IN_MS);
        
        this->ui->showWarning();
        this->fsm.trigger(Event::DISCARD_MESSAGE);
    }
}

void Application::sendLastMessage() 
{
    this->ui->isBusy(true);

    logger.trace(F("Sending message '%s' to '%s'"), currentRecordingName, config.getPostMessageUrl().c_str());

    f = SD.open(currentRecordingName, FILE_READ);
    logger.trace(F("Message size: %i bytes"), f.size());

    uploader->send(&f, config.getPostMessageUrl().c_str());
}

void Application::whileMessageSending() 
{
    if (uploader->isCompleted()) {
        currentBytesSent = 0;
        f.close();
        
        bool wasSuccessful = uploader->isSuccessful();
        logger.trace(F("Message is sent. Successful: %s"), wasSuccessful ? "yes" : "no");

        if (wasSuccessful) {
            this->ui->showSuccess();
        }
        else {
            this->ui->showError();
        }

        this->fsm.trigger(Event::MESSAGE_SENT);
        return;  
    }

    if (this->uploader->getBytesSent() != currentBytesSent) {
        currentBytesSent = this->uploader->getBytesSent();
        long total = this->uploader->getBytesTotal();
        int percent = (int)((currentBytesSent * 100.0f) / total);
        logger.verbose("Upload progress: %i/%i, %i%%", currentBytesSent, total, percent);
    }
}

void Application::playMessageFrom(int buttonId) 
{
    
}

void Application::whileMessagePlaying() 
{
    if (millis() % 100 == 0) {
        this->fsm.trigger(Event::MESSAGE_PLAYED);
    }
}