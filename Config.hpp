#pragma once
#include "settings/Settings.h"

using namespace std;

class Config : public SettingsAbstract {
public:
    SETTINGS_PARAMETER(bool, ENABLED, "MAIN/ENABLED", false);
};
