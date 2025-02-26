#ifndef EC800_BOARD_H
#define EC800_BOARD_H

#include "board.h"
#include <ec800_at_modem.h>

class EC800Board : public Board {
protected:
    EC800AtModem modem_;

    virtual std::string GetBoardJson() override;
    void WaitForNetworkReady();

public:
    EC800Board(gpio_num_t tx_pin, gpio_num_t rx_pin, size_t rx_buffer_size = 4096);
    virtual std::string GetBoardType() override;
    virtual void StartNetwork() override;
    virtual Http* CreateHttp() override;
    virtual WebSocket* CreateWebSocket() override;
    virtual Mqtt* CreateMqtt() override;
    virtual Udp* CreateUdp() override;
    virtual const char* GetNetworkStateIcon() override;
    virtual void SetPowerSaveMode(bool enabled) override;
};

#endif // EC800_BOARD_H
