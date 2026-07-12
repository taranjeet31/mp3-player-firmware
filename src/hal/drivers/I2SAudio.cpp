#include "I2SAudio.h"
#include "Logging.h"
#include "config.h"
#include <SD_MMC.h>

extern volatile uint32_t audioLoopIterations;
extern volatile uint32_t lastAudioLoopReturnMs;

I2SAudio::I2SAudio() : volume(10), playing(false), cached_duration(0), cached_elapsed(0), commandQueue(nullptr), stateMutex(nullptr), lastSeekSeconds(0xFFFF), lastSeekMs(0) {}

bool I2SAudio::init() {
    commandQueue = xQueueCreate(10, sizeof(AudioCommand));
    if (commandQueue == nullptr) {
        Serial.println("[Audio] Error: Failed to create command queue!");
        return false;
    }
    stateMutex = xSemaphoreCreateMutex();
    if (stateMutex == nullptr) {
        Serial.println("[Audio] Error: Failed to create state mutex!");
        return false;
    }

    audio.setPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
    audio.setVolume(volume); // range 0-21
    return true;
}

void I2SAudio::queueCommand(const AudioCommand& cmd) {
    if (commandQueue) {
        xQueueSend(commandQueue, &cmd, 0);
    }
}

void I2SAudio::setStatePlaying(bool play) {
    if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        playing = play;
        xSemaphoreGive(stateMutex);
    }
}

void I2SAudio::setStateVolume(uint8_t vol) {
    if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        volume = vol;
        xSemaphoreGive(stateMutex);
    }
}

void I2SAudio::setStateCachedDuration(uint32_t dur) {
    if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        cached_duration = dur;
        xSemaphoreGive(stateMutex);
    }
}

void I2SAudio::setStateCachedElapsed(uint32_t elap) {
    if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        cached_elapsed = elap;
        xSemaphoreGive(stateMutex);
    }
}

void I2SAudio::processCommands() {
    AudioCommand cmd;
    while (commandQueue && xQueueReceive(commandQueue, &cmd, 0) == pdTRUE) {
        switch (cmd.type) {
            case AudioCommandType::PLAY_FILE: {
                logPrintf("[AudioTask] Executing PLAY_FILE: %s\n", cmd.path);
                if (audio.connecttoFS(SD_MMC, cmd.path)) {
                    setStatePlaying(true);
                } else {
                    logPrintf("[AudioTask] Failed to open file: %s\n", cmd.path);
                    setStatePlaying(false);
                }
                setStateCachedDuration(0);
                setStateCachedElapsed(0);
                break;
            }
            case AudioCommandType::STOP: {
                logPrintf("[AudioTask] Executing STOP\n");
                audio.stopSong();
                setStatePlaying(false);
                setStateCachedDuration(0);
                setStateCachedElapsed(0);
                break;
            }
            case AudioCommandType::SET_VOLUME: {
                logPrintf("[AudioTask] Executing SET_VOLUME: %d\n", cmd.arg.volume);
                audio.setVolume(cmd.arg.volume);
                setStateVolume(cmd.arg.volume);
                break;
            }
            case AudioCommandType::SEEK: {
                uint16_t target = cmd.arg.seek_seconds;
                uint32_t now = millis();
                if (!playing) {
                    logPrintf("[AudioTask] Seek rejected: playback is not active\n");
                    break;
                }
                if (cached_duration == 0) {
                    logPrintf("[AudioTask] Seek rejected: track duration is unknown\n");
                    break;
                }
                if (target >= cached_duration) {
                    logPrintf("[AudioTask] Seek rejected: target %u is >= duration %u\n", target, cached_duration);
                    break;
                }
                if (target < 3) {
                    logPrintf("[AudioTask] Seek rejected: target %u is within the first 3 seconds\n", target);
                    break;
                }
                if (target == lastSeekSeconds) {
                    logPrintf("[AudioTask] Seek rejected: duplicate request of %u seconds\n", target);
                    break;
                }
                if (now - lastSeekMs < 500) {
                    logPrintf("[AudioTask] Seek rejected: too frequent (less than 500ms since last seek)\n");
                    break;
                }

                logPrintf("[AudioTask] Executing SEEK: %d seconds\n", target);
                audio.setAudioPlayPosition(target);
                lastSeekSeconds = target;
                lastSeekMs = now;
                break;
            }
        }
    }
}

void I2SAudio::playFile(const char* filepath) {
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_FILE;
    if (filepath) {
        strncpy(cmd.path, filepath, sizeof(cmd.path) - 1);
        cmd.path[sizeof(cmd.path) - 1] = '\0';
    } else {
        cmd.path[0] = '\0';
    }
    queueCommand(cmd);
}

void I2SAudio::stop() {
    AudioCommand cmd;
    cmd.type = AudioCommandType::STOP;
    queueCommand(cmd);
}

void I2SAudio::setVolume(uint8_t vol) {
    if (vol > 21) vol = 21;
    AudioCommand cmd;
    cmd.type = AudioCommandType::SET_VOLUME;
    cmd.arg.volume = vol;
    queueCommand(cmd);
}

uint8_t I2SAudio::getVolume() const {
    uint8_t val = 10;
    if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        val = volume;
        xSemaphoreGive(stateMutex);
    }
    return val;
}

void I2SAudio::loop() {
    processCommands();

    audio.loop();

    audioLoopIterations++;
    lastAudioLoopReturnMs = millis();

    bool currentPlaying = audio.isRunning();
    setStatePlaying(currentPlaying);
    
    static uint32_t last_query_time = 0;
    uint32_t now = millis();
    if (now - last_query_time >= 250) {
        last_query_time = now;
        if (currentPlaying) {
            setStateCachedDuration(audio.getAudioFileDuration());
            setStateCachedElapsed(audio.getAudioCurrentTime());
        } else {
            setStateCachedElapsed(0);
            setStateCachedDuration(0);
        }
    }
}

uint32_t I2SAudio::getAudioFileDuration() {
    uint32_t val = 0;
    if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        val = cached_duration;
        xSemaphoreGive(stateMutex);
    }
    return val;
}

uint32_t I2SAudio::getAudioCurrentTime() {
    uint32_t val = 0;
    if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        val = cached_elapsed;
        xSemaphoreGive(stateMutex);
    }
    return val;
}

void I2SAudio::setAudioPlayPosition(uint16_t seconds) {
    AudioCommand cmd;
    cmd.type = AudioCommandType::SEEK;
    cmd.arg.seek_seconds = seconds;
    queueCommand(cmd);
}

bool I2SAudio::isPlaying() const {
    bool val = false;
    if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        val = playing;
        xSemaphoreGive(stateMutex);
    }
    return val;
}

// Global ESP32-audioI2S callback implementations to prevent null-pointer function calls
void audio_info(const char *info) {
    logPrintf("[Audio Info] %s\n", info);
}

void audio_id3data(const char *info) {
    logPrintf("[Audio ID3] %s\n", info);
}

void audio_eof_mp3(const char *info) {
    logPrintf("[Audio EOF] %s\n", info);
}

void audio_showstation(const char *info) {
    logPrintf("[Audio Station] %s\n", info);
}

void audio_showstreaminfo(const char *info) {
    logPrintf("[Audio Stream] %s\n", info);
}

void audio_showstreamtitle(const char *info) {
    logPrintf("[Audio Title] %s\n", info);
}

void audio_bitrate(const char *info) {
    logPrintf("[Audio Bitrate] %s\n", info);
}

void audio_commercial(const char *info) {
    logPrintf("[Audio Commercial] %s\n", info);
}

void audio_icyurl(const char *info) {
    logPrintf("[Audio ICY URL] %s\n", info);
}

void audio_icydescription(const char *info) {
    logPrintf("[Audio ICY Desc] %s\n", info);
}

void audio_lasthost(const char *info) {
    logPrintf("[Audio Last Host] %s\n", info);
}
