#ifndef _UISTATE_H
#define _UISTATE_H

class UiState {
    
    public:
        enum Value : uint8_t { MAIN, MENU, WIFI, WIFI_ENABLE, CO2, CO2_ENABLE };
        
        UiState() = default;
        constexpr UiState(Value aUiState) : value(aUiState) { }

        operator Value() const { return value; }
        //explicit operator bool() = delete;
        
        constexpr bool operator==(UiState a) const { return value == a.value; }
        constexpr bool operator!=(UiState a) const { return value != a.value; }
    

        constexpr bool hasMenu() const { return value != MAIN; }

    private:
        Value value;
};

#endif