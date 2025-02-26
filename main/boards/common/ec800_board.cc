#include "ec800_board.h"

#include "application.h"
#include "display.h"
#include "font_awesome_symbols.h"
#include "assets/lang_config.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <ec800_http.h>
#include <ec800_ssl_transport.h>
#include <web_socket.h>
#include <ec800_mqtt.h>
#include <ec800_udp.h>
#include <opus_encoder.h>

static const char *TAG = "EC800Board";

EC800Board::EC800Board(gpio_num_t tx_pin, gpio_num_t rx_pin, size_t rx_buffer_size) : modem_(tx_pin, rx_pin, rx_buffer_size) {
}

std::string EC800Board::GetBoardType() {
    return "ec800";
}

void EC800Board::StartNetwork() {
    auto display = Board::GetInstance().GetDisplay();
    display->SetStatus(Lang::Strings::DETECTING_MODULE);
    modem_.SetDebug(false);
    modem_.SetBaudRate(115200);

    auto& application = Application::GetInstance();
    // If low power, the material ready event will be triggered by the modem because of a reset
    modem_.OnMaterialReady([this, &application]() {
        ESP_LOGI(TAG, "EC800 material ready");
        application.Schedule([this, &application]() {
            application.SetDeviceState(kDeviceStateIdle);
            WaitForNetworkReady();
        });
    });

    WaitForNetworkReady();
}

void EC800Board::WaitForNetworkReady() {
    auto& application = Application::GetInstance();
    auto display = Board::GetInstance().GetDisplay();
    display->SetStatus(Lang::Strings::REGISTERING_NETWORK);
    int result = modem_.WaitForNetworkReady();
    if (result == -1) {
        application.Alert(Lang::Strings::ERROR, Lang::Strings::PIN_ERROR, "sad", Lang::Sounds::P3_ERR_PIN);
        return;
    } else if (result == -2) {
        application.Alert(Lang::Strings::ERROR, Lang::Strings::REG_ERROR, "sad", Lang::Sounds::P3_ERR_REG);
        return;
    }

    // Print the EC800 modem information
    std::string module_name = modem_.GetModuleName();
    std::string imei = modem_.GetImei();
    std::string iccid = modem_.GetIccid();
    ESP_LOGI(TAG, "EC800 Module: %s", module_name.c_str());
    ESP_LOGI(TAG, "EC800 IMEI: %s", imei.c_str());
    ESP_LOGI(TAG, "EC800 ICCID: %s", iccid.c_str());

    // Close all previous connections
    // modem_.ResetConnections();
}

Http* EC800Board::CreateHttp() {
    return new EC800Http(modem_);
}

WebSocket* EC800Board::CreateWebSocket() {
    return new WebSocket(new EC800SslTransport(modem_, 0));
}

Mqtt* EC800Board::CreateMqtt() {
    return new EC800Mqtt(modem_, 0);
}

Udp* EC800Board::CreateUdp() {
    return new EC800Udp(modem_, 0);
}

const char* EC800Board::GetNetworkStateIcon() {
    if (!modem_.network_ready()) {
        return FONT_AWESOME_SIGNAL_OFF;
    }
    int csq = modem_.GetCsq();
    if (csq == -1) {
        return FONT_AWESOME_SIGNAL_OFF;
    } else if (csq >= 0 && csq <= 14) {
        return FONT_AWESOME_SIGNAL_1;
    } else if (csq >= 15 && csq <= 19) {
        return FONT_AWESOME_SIGNAL_2;
    } else if (csq >= 20 && csq <= 24) {
        return FONT_AWESOME_SIGNAL_3;
    } else if (csq >= 25 && csq <= 31) {
        return FONT_AWESOME_SIGNAL_4;
    }

    ESP_LOGW(TAG, "Invalid CSQ: %d", csq);
    return FONT_AWESOME_SIGNAL_OFF;
}

std::string EC800Board::GetBoardJson() {
    // Set the board type for OTA
    std::string board_type = BOARD_TYPE;
    std::string board_json = std::string("{\"type\":\"" + board_type + "\",");
    board_json += "\"revision\":\"" + modem_.GetModuleName() + "\",";
    board_json += "\"carrier\":\"" + modem_.GetCarrierName() + "\",";
    board_json += "\"csq\":\"" + std::to_string(modem_.GetCsq()) + "\",";
    board_json += "\"imei\":\"" + modem_.GetImei() + "\",";
    board_json += "\"iccid\":\"" + modem_.GetIccid() + "\"}";
    return board_json;
}

void EC800Board::SetPowerSaveMode(bool enabled) {
    // TODO: Implement power save mode for EC800
}
