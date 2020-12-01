#ifndef _UISTATE_H
#define _UISTATE_H

class UiState {
    
    public:
        enum Value : uint8_t {
             MAIN, MENU, WIFI, WIFI_ENABLE, CO2, CO2_ENABLE,
             WIFI_SCAN, GEIGER, GEIGER_ENABLE, METEOSENSOR, AUDIO };
        
        //operator Value() const { return value; }
        //explicit operator bool() = delete;
        
        //constexpr bool operator==(UiState a) const { return value == a.value; }
        //constexpr bool operator!=(UiState a) const { return value != a.value; }

        bool hasMenu() const { return value != MAIN; }

        void setOnStateChangeListener(void(*fn)()) {
            onUiStasteChanged = fn;
        }

        Value getCurrent() { return value; }
        void setUiState(Value state) { 
            value = state; 
            if (onUiStasteChanged) {
                onUiStasteChanged();
            }
        }

        void setMenuItemCounter(int counter) {
            menuItemsCounter = counter;
        }

        char ** getMenu() { return menuItems; }

        int getMenuItemsCounter() { return menuItemsCounter; }

        int getSelectedMenuItem() { return selectedMenuItem; }

        void onEncoderEncrease() {
            if (selectedMenuItem < menuItemsCounter - 1) selectedMenuItem++;
        }

        void onEncoderDecrease() {
            if (selectedMenuItem > 0 ) selectedMenuItem--;
        }

        void onButtonPressed() {
            switch (value) {
                case UiState::MENU: {
                    switch (selectedMenuItem) {
                        case 0:
                            showMainScreen();
                            break;
                        case 1:
                            showWifiScren();
                            break;
                        case 2: 
                            showGeigerScren();
                            break;
                        case 3: 
                            showCo2Scren();
                            break;
                        case 4:
                            showMeteoSensorScren();
                            break;
                        case 5: 
                            showBtAudioScreen();
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case UiState::MAIN: {
                    showMenuScreen();
                    break;
                }
                case UiState::METEOSENSOR: {
                    switch (selectedMenuItem) {
                        case 0:
                            showMainScreen();
                            break;
                    }
                    break;
                }
                case UiState::WIFI: {
                    switch (selectedMenuItem) {
                        case 0:
                            showMenuScreen();
                            break;
                        case 1: 
                            showEnableWifiScreen();
                            break;
                        case 3:
                            showScanWifiScreen();
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case UiState::WIFI_ENABLE: {
                    showWifiScren();
                    break;
                }
                case UiState::WIFI_SCAN: {
                switch (selectedMenuItem) {
                    case 0: {
                        showWifiScren();
                        break;
                    }
                    default:
                        break;
                }
                break;
                }
                case UiState::CO2: {
                switch (selectedMenuItem) {
                case 0:
                    showMenuScreen();
                    break;
                case 1: 
                    showEnableCo2Scren();
                    break;
                }
                break;
                }
                case UiState::CO2_ENABLE: {
                    showCo2Scren();
                    break;
                }
                case UiState::GEIGER: {
                    switch (selectedMenuItem) {
                        case 0:
                            showMenuScreen();
                            break;
                        case 1: 
                            showEnableGeigerScreen();
                            break;
                    }
                    break;
                }
                case UiState::GEIGER_ENABLE: {
                    showGeigerScren();
                    break;
                }
                case UiState::AUDIO: {
                switch (selectedMenuItem) {
                    case 0:
                        showMenuScreen();
                        break;
                    default:
                        break;
                }
                break;
                }
            }
            if (onUiStasteChanged) {
                onUiStasteChanged();
            }
        }

    private:
        Value value;
        void(*onUiStasteChanged)();
        uint8_t menuItemsCounter = 0;
        uint8_t selectedMenuItem = 0;
        char** menuItems;

        void showMainScreen() {
            value = MAIN;
            //delete[] menuItems; // NPE
            selectedMenuItem = 0;
            menuItemsCounter = 0;
        }

        void showMenuScreen() {
            value = MENU;
            delete[] menuItems;
            menuItems = new char*[6] {" <- back"," Wifi", " Geiger", " Co2", " Meteo sensor", " Audio"};
            selectedMenuItem = 0;
            menuItemsCounter = 6;
        }

        void showWifiScren() {
            value = WIFI;
            delete[] menuItems;
            menuItems = new char*[4] {" <- back"," on/off", " Connect", " Scanner"};
            selectedMenuItem = 0;
            menuItemsCounter = 4;
        }

        void showEnableWifiScreen() {
            value = WIFI_ENABLE;
            delete[] menuItems;
            menuItems = new char*[3] {" <- back"," Enable", " Disable"};
            menuItemsCounter = 3;
            selectedMenuItem = 0; //wifiEnabled ? 2 : 1;
        }

        void showScanWifiScreen() {
            menuItemsCounter = 0;
            selectedMenuItem = 0;            
            value = WIFI_SCAN;
        }

        void showCo2Scren() {
            value = CO2;
            delete[] menuItems;
            menuItems = new char*[2] {" <- back"," on/off"};
            selectedMenuItem = 0;
            menuItemsCounter = 2;
        }

        void showMeteoSensorScren() {
            value = METEOSENSOR;
            delete[] menuItems;
            menuItems = new char*[4] {" <- back", " Temperature", " Pressure", " Humidity"};
            selectedMenuItem = 1;
            menuItemsCounter = 4;
        }

        void showEnableCo2Scren() {
            value = CO2_ENABLE;
            delete[] menuItems;
            menuItems = new char*[3] {" <- back"," Enable", " Disable"};
            menuItemsCounter = 3;
            selectedMenuItem = 1;//co2Enabled ? 2 : 1;
        }

        void showGeigerScren() {
            value = UiState::GEIGER;
            delete[] menuItems;
            menuItems = new char*[2] {" <- back"," on/off"};
            menuItemsCounter = 2;
            selectedMenuItem = 0;
        }

        void showEnableGeigerScreen() {
            value = GEIGER_ENABLE;
            delete[] menuItems;
            menuItems = new char*[3] {" <- back"," Enable", " Disable"};
            menuItemsCounter = 3;
            selectedMenuItem = 1;//geigerEnabled ? 2 : 1;
        }

        void showBtAudioScreen() {
            value = AUDIO;
            delete[] menuItems;
            menuItems = new char*[3] {" <- back"," Enable", " Disable"};
            menuItemsCounter = 3;
            selectedMenuItem = 0;
        }


};

#endif