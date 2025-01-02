#pragma once

#include <lvgl.h>
#include <TFT_eSPI.h>

#include "ScreenPage.hpp"


// 主页面
extern std::shared_ptr<ScreenPage> RootPage;

// 设置DDS参数菜单页
extern std::shared_ptr<ScreenPage> SetDDSPage;
// 各设置项目页
extern std::shared_ptr<ScreenPage> SetWaveFormPage;
extern std::shared_ptr<ScreenPage> SetFrequencyPage;
extern std::shared_ptr<ScreenPage> SetPhasePage;

// 其他设置菜单页
extern std::shared_ptr<ScreenPage> OtherSettingsPage;


extern void loopLvglDisplay(void);
extern void initLvglDisplay(void);