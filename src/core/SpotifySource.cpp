#if defined(TARGET_THMI)
#include "core/SpotifySource.h"
#include "config.h"
#include "Logging.h"

// cspot includes
#include "CSpotContext.h"
#include "LoginBlob.h"
#include "SpircHandler.h"
#include "TrackPlayer.h"
#include "BellHTTPServer.h"
#include "CircularBuffer.h"
#include "MDNSService.h"
#include "Logger.h"
#include "civetweb.h"
#include "nlohmann/json.hpp"

// ESP32 includes
#include <driver/i2s.h>
#include <ESPmDNS.h>

SpotifySource::SpotifySource()
    : cspotTaskHandle(nullptr),
      audioTaskHandle(nullptr),
      active(false),
      isPlayingState(false),
      isPausedState(true),
      isCspotInitialized(false),
      currentTrackName("Spotify Connect"),
      currentArtistName("Ready to stream"),
      trackDurationSec(0),
      lastPositionUpdateMs(0),
      lastPositionTimeMs(0),
      volume(10) {
    circularBuffer = std::make_unique<bell::CircularBuffer>(1024 * 128 * 4); // 512KB buffer for PSRAM
}

SpotifySource::~SpotifySource() {
    deactivate();
}

void SpotifySource::initI2S() {
    logPrintln("[SpotifySource] Initializing direct ESP-IDF I2S driver...");
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK_PIN,
        .ws_io_num = I2S_LRC_PIN,
        .data_out_num = I2S_DOUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err == ESP_OK) {
        i2s_set_pin(I2S_NUM_0, &pin_config);
        i2s_zero_dma_buffer(I2S_NUM_0);
        logPrintln("[SpotifySource] I2S driver installed successfully.");
    } else {
        logPrintf("[SpotifySource] ERROR: Failed to install I2S driver (0x%X)\n", err);
    }
}

void SpotifySource::deinitI2S() {
    logPrintln("[SpotifySource] De-initializing direct ESP-IDF I2S driver...");
    i2s_driver_uninstall(I2S_NUM_0);
}

void SpotifySource::activate() {
    if (active) return;
    
    logPrintln("[SpotifySource] Activating Spotify Connect Source...");
    active = true;
    isPlayingState = false;
    isPausedState = true;
    currentTrackName = "Spotify Connect";
    currentArtistName = "Ready to stream";
    trackDurationSec = 0;
    lastPositionUpdateMs = 0;
    lastPositionTimeMs = millis();
    
    // 1. Re-initialize I2S for direct write
    initI2S();
    
    // 2. Start background Spotify worker threads on Core 0 (priority 1)
    xTaskCreatePinnedToCore(
        cspotThread,
        "cspotThread",
        16 * 1024,
        this,
        1,
        &cspotTaskHandle,
        0
    );
    
    // 3. Start audio direct-write rendering task on Core 0 (priority 2)
    xTaskCreatePinnedToCore(
        audioThread,
        "spotifyAudio",
        8 * 1024,
        this,
        2,
        &audioTaskHandle,
        0
    );
}

void SpotifySource::deactivate() {
    if (!active) return;
    
    logPrintln("[SpotifySource] Deactivating Spotify Connect Source...");
    active = false;
    isCspotInitialized = false;
    
    // Signal disconnect if running
    if (handler) {
        handler->disconnect();
    }
    
    // Stop tasks and wait for termination
    if (cspotTaskHandle) {
        vTaskDelete(cspotTaskHandle);
        cspotTaskHandle = nullptr;
    }
    if (audioTaskHandle) {
        vTaskDelete(audioTaskHandle);
        audioTaskHandle = nullptr;
    }
    
    // Release network resources
    server.reset();
    ctx.reset();
    handler.reset();
    
    // Uninstall direct I2S configuration (allows Local SD card to regain control)
    deinitI2S();
}

void SpotifySource::loop() {
    // Periodic housekeeping if any
}

void SpotifySource::play() {
    if (handler) {
        handler->setPause(false);
    }
}

void SpotifySource::pause() {
    if (handler) {
        handler->setPause(true);
    }
}

void SpotifySource::stop() {
    pause();
}

void SpotifySource::next() {
    if (handler) {
        handler->nextSong();
    }
}

void SpotifySource::prev() {
    if (handler) {
        handler->previousSong();
    }
}

void SpotifySource::setVolume(uint8_t vol) {
    if (vol > 21) vol = 21;
    volume = vol;
    
    // Scale 0-21 UI volume to Spotify volume range (0 to 65535)
    uint32_t spotifyVol = (uint32_t)((vol * 65535) / 21);
    if (handler) {
        handler->setRemoteVolume(spotifyVol);
    }
}

uint8_t SpotifySource::getVolume() const {
    return volume;
}

bool SpotifySource::isPlaying() const {
    return isPlayingState;
}

bool SpotifySource::isPaused() const {
    return isPausedState;
}

String SpotifySource::getTrackName() const {
    return String(currentTrackName.c_str());
}

String SpotifySource::getArtistName() const {
    return String(currentArtistName.c_str());
}

uint32_t SpotifySource::getTrackDuration() const {
    return trackDurationSec;
}

uint32_t SpotifySource::getTrackElapsed() const {
    if (isPausedState || !isPlayingState) {
        return lastPositionUpdateMs / 1000;
    }
    uint32_t diff = (millis() - lastPositionTimeMs) / 1000;
    uint32_t elapsed = (lastPositionUpdateMs / 1000) + diff;
    if (elapsed > trackDurationSec) {
        elapsed = trackDurationSec;
    }
    return elapsed;
}

void SpotifySource::seekTo(uint16_t seconds) {
    if (handler) {
        // Spotify expects seek position in milliseconds
        uint32_t ms = seconds * 1000;
        handler->updatePositionMs(ms);
    }
}

void SpotifySource::feedData(uint8_t* data, size_t bytes) {
    size_t toWrite = bytes;
    while (active && toWrite > 0) {
        size_t written = circularBuffer->write(data + (bytes - toWrite), toWrite);
        if (written == 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        toWrite -= written;
    }
}

void SpotifySource::handleEvent(std::unique_ptr<cspot::SpircHandler::Event> event) {
    if (!event) return;
    
    switch (event->eventType) {
        case cspot::SpircHandler::EventType::PLAY_PAUSE: {
            bool paused = std::get<bool>(event->data);
            if (paused) {
                lastPositionUpdateMs = getTrackElapsed() * 1000;
            }
            isPausedState = paused;
            isPlayingState = !paused;
            lastPositionTimeMs = millis();
            logPrintf("[SpotifySource] Play/Pause Event: paused=%d\n", (int)paused);
            break;
        }
        case cspot::SpircHandler::EventType::TRACK_INFO: {
            auto trackInfo = std::get<cspot::TrackInfo>(event->data);
            currentTrackName = trackInfo.name;
            currentArtistName = trackInfo.artist;
            trackDurationSec = trackInfo.duration / 1000;
            lastPositionUpdateMs = 0;
            lastPositionTimeMs = millis();
            logPrintf("[SpotifySource] Track Changed Event: '%s' by '%s' (%u seconds)\n", 
                      currentTrackName.c_str(), currentArtistName.c_str(), (unsigned)trackDurationSec);
            break;
        }
        case cspot::SpircHandler::EventType::VOLUME: {
            int vol = std::get<int>(event->data);
            uint8_t scaledVol = (uint8_t)((vol * 21) / 65535);
            volume = scaledVol;
            logPrintf("[SpotifySource] Spotify Volume Sync: %d (scaled: %d)\n", vol, scaledVol);
            break;
        }
        case cspot::SpircHandler::EventType::SEEK: {
            int seekMs = std::get<int>(event->data);
            lastPositionUpdateMs = seekMs;
            lastPositionTimeMs = millis();
            circularBuffer->emptyBuffer();
            logPrintf("[SpotifySource] Spotify Seek Sync: %d ms\n", seekMs);
            break;
        }
        case cspot::SpircHandler::EventType::FLUSH: {
            circularBuffer->emptyBuffer();
            logPrintln("[SpotifySource] Buffer Flush Event");
            break;
        }
        case cspot::SpircHandler::EventType::PLAYBACK_START: {
            circularBuffer->emptyBuffer();
            isPausedState = false;
            isPlayingState = true;
            lastPositionUpdateMs = 0;
            lastPositionTimeMs = millis();
            logPrintln("[SpotifySource] Playback Started Event");
            break;
        }
        default:
            break;
    }
}

void SpotifySource::cspotThread(void* param) {
    auto* self = static_cast<SpotifySource*>(param);
    
    // Initialize standard MDNS library using Arduino ESPmDNS
    MDNS.begin("cspot");
    
    // Set Spotify connect device name
    auto blob = std::make_shared<cspot::LoginBlob>("LilyGo Spotify Connect");
    
    // Initialize Civetweb web server in cspot
    self->server = std::make_unique<bell::BellHTTPServer>(8080);
    
    self->server->registerGet(
        "/spotify_info", [self, blob](struct mg_connection* conn) {
            return self->server->makeJsonResponse(blob->buildZeroconfInfo());
        }
    );
    
    std::atomic<bool> gotBlob = false;
    self->server->registerPost(
        "/spotify_info", [self, blob, &gotBlob](struct mg_connection* conn) {
            nlohmann::json obj;
            obj["status"] = 101;
            obj["spotifyError"] = 0;
            obj["statusString"] = "ERROR-OK";

            std::string body = "";
            auto requestInfo = mg_get_request_info(conn);
            if (requestInfo->content_length > 0) {
                body.resize(requestInfo->content_length);
                mg_read(conn, body.data(), requestInfo->content_length);

                mg_header hd[10];
                int num = mg_split_form_urlencoded(body.data(), hd, 10);
                std::map<std::string, std::string> queryMap;

                for (int i = 0; i < num; i++) {
                    queryMap[hd[i].name] = hd[i].value;
                }

                blob->loadZeroconfQuery(queryMap);
                gotBlob = true;
            }

            return self->server->makeJsonResponse(obj.dump());
        }
    );

    // Register mdns service type
    bell::MDNSService::registerService(
        blob->getDeviceName(), "_spotify-connect", "_tcp", "", 8080,
        {{"VERSION", "1.0"}, {"CPath", "/spotify_info"}, {"Stack", "SP"}}
    );

    logPrintln("[SpotifySource] mDNS Zeroconf registration complete. Discovery active.");

    while (self->active) {
        if (!gotBlob) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        logPrintln("[SpotifySource] Device discovery completed. Authenticating...");
        
        try {
            self->ctx = cspot::Context::createFromBlob(blob);
            self->ctx->session->connectWithRandomAp();
            auto token = self->ctx->session->authenticate(blob);
            
            if (token.size() > 0) {
                logPrintln("[SpotifySource] Connected to Spotify Core servers successfully.");
                self->ctx->session->startTask();
                
                self->handler = std::make_shared<cspot::SpircHandler>(self->ctx);
                self->handler->subscribeToMercury();
                
                self->handler->getTrackPlayer()->setDataCallback(
                    [self](uint8_t* data, size_t bytes, std::string_view format) -> size_t {
                        self->feedData(data, bytes);
                        return bytes;
                    }
                );
                
                self->handler->setEventHandler(
                    [self](std::unique_ptr<cspot::SpircHandler::Event> event) {
                        self->handleEvent(std::move(event));
                    }
                );
                
                self->isCspotInitialized = true;
                
                while (self->active && gotBlob) {
                    self->ctx->session->handlePacket();
                }
                
                self->isCspotInitialized = false;
                self->handler->disconnect();
                self->handler.reset();
            } else {
                logPrintln("[SpotifySource] Authentication error. resetting discovery state.");
                gotBlob = false;
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
        } catch (const std::exception& e) {
            logPrintf("[SpotifySource] Runtime Exception: %s\n", e.what());
            gotBlob = false;
            self->isCspotInitialized = false;
            vTaskDelay(pdMS_TO_TICKS(3000));
        } catch (...) {
            logPrintln("[SpotifySource] Unknown context error occurred.");
            gotBlob = false;
            self->isCspotInitialized = false;
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
    
    self->cspotTaskHandle = nullptr;
    vTaskDelete(NULL);
}

void SpotifySource::audioThread(void* param) {
    auto* self = static_cast<SpotifySource*>(param);
    std::vector<uint8_t> buffer(1024);
    
    while (self->active) {
        if (self->isPlayingState && !self->isPausedState) {
            size_t readBytes = self->circularBuffer->read(buffer.data(), buffer.size());
            if (readBytes > 0) {
                size_t bytesWritten = 0;
                i2s_write(I2S_NUM_0, buffer.data(), readBytes, &bytesWritten, portMAX_DELAY);
            } else {
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
    
    self->audioTaskHandle = nullptr;
    vTaskDelete(NULL);
}
#endif
