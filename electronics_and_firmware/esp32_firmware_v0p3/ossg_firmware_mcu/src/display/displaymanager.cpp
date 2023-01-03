//=====================================================
//  THIS FILE TO BE USED FOR INTERFACING WITH DISPLAY
//=====================================================

//#pragma once
using namespace std;
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"
#include <iostream>
#include <driver/i2c.h>
#include <iomanip>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"

#define LGFX_USE_V1
#define LGFX_CVBS
#define LGFX_ESP_WROVER_KIT
#define DISPLAY_EN_PIN GPIO_NUM_2
#define MAX_CAPTION_LENGTH 6000
#include <LovyanGFX.hpp>
#include "user_setting.hpp"
#include "helper_display.hpp"
#include <gui/ui_ext.hpp>
#include "message_types.hpp"
#define LOVYAN_USE_CORE_NUM 1
static const char *TAG = "lvgl_gui";

// MessageTypes messageTypesList;
// char * currentMode = strdup(messageTypesList.MODE_HOME);

//esp_timer_handle_t screenTimer;
static void screenTimerCallback(void* arg);
esp_timer_handle_t screenTimer;
esp_timer_create_args_t screenTimerArgs = {
    .callback = &screenTimerCallback,
    /* argument specified here will be passed to timer callback function */
    //.arg = (void*) periodic_timer,
    .name = "one-shot"
};

lv_obj_t* blankScreen;

//display power control
void setup_display_en(void){
    gpio_set_direction(DISPLAY_EN_PIN, GPIO_MODE_OUTPUT);   
}

void power_to_display(bool power_on){
    ESP_LOGI(TAG, "Sending power value to display: %d", power_on);
    gpio_set_level(DISPLAY_EN_PIN, !power_on);         // Turn the LED on
}

// Set everything up
void displayStart(){
    cout << "DISPLAY START TRIGGERED" << endl;
    
    // Turn on power to the display
    setup_display_en();
    power_to_display(true);

    lcd.init();        // Initialize LovyanGFX
    
    lv_init();         // Initialize lvgl

    if (lv_display_init() != ESP_OK) // Configure LVGL
    {
        ESP_LOGE(TAG, "LVGL setup failed!!!");
    }
    
    blankScreen = lv_obj_create(NULL);
    
    // Function generated by squareline studio to register all its ui components
    ui_init_custom();

    esp_timer_create(&screenTimerArgs, &screenTimer);
}

/*
    SCREEN ON / OFF FUNCTIONS
*/

//Wipe everything from screen
void displayClear(){
    lvgl_acquire();
    lv_scr_load(blankScreen);
    lcd.setBrightness(0);
    lvgl_release(); 
}

void screenTimerCallback(void* arg){
    ESP_LOGI(TAG, "SCREEN TIMER CALLBACK TRIGGERED");
    displayClear();
    power_to_display(false);
}

void updateClock(){
    /*
    TODO: Handle this.
    Currently there are multiple clock objects so either change them all or find a way to consolidate them.
    (Do this next commit)
    */
}

// Call this when turning on or updating the screen 
// (updates the screen shutoff timer)
void updateActivity(int timeoutSeconds = 5){
    ESP_LOGI(TAG, "UPDATE ACTIVITY TRIGGERED");
    power_to_display(true); //TODO: evaluate putting this here
    lcd.setBrightness(128);

    uint64_t timeoutMicroSeconds = timeoutSeconds * 1000000;
    if(timeoutSeconds == -1) timeoutMicroSeconds = INT64_MAX;
    esp_timer_start_once(screenTimer, timeoutMicroSeconds);
}

/*
    HELPER FUNCTIONS TO DISPLAY VARIOUS SCREENS
*/

// To be called when the wake word is detected (or maybe on button press) 
void displayEnterVoiceCommandStep1(){
    lvgl_acquire();
    lv_scr_load(ui_Enter_Voice_Command);
    lvgl_release();
}

void displayEnterVoiceCommandStep2(){
    lvgl_acquire();
    lv_scr_load(ui_Enter_Voice_Command_2);
    lvgl_release();
}

//TODO: implement this in a decent way
void displayEnterVoiceCommandStep3(char* command, char* soFar){
    lvgl_acquire();
    lv_scr_load(ui_Enter_Voice_Command_3);
    lvgl_release();
}

string previousTitle = "";
string currentCaptionString = "";
void displayLiveCaptions(char* title = "", char* body = ""){
    ESP_LOGI(TAG, "DISP LIVE CAPTIONS CALLED WITH TITLE: %s, BODY: %s", title, body);
    string titleString(title);
    string bodyString(body);
    if(titleString != previousTitle) currentCaptionString = "";
    if(currentCaptionString.length() > 0) currentCaptionString += "\n\n";
    previousTitle = titleString;
    currentCaptionString = currentCaptionString + bodyString;
    //ESP_LOGI(TAG, "NEW CAPTION: " + currentCaptionString);
    if(currentCaptionString.length() > MAX_CAPTION_LENGTH) currentCaptionString = currentCaptionString.substr(currentCaptionString.length()/2);

    updateActivity(-1);
    lvgl_acquire();
    if(lv_scr_act() == ui_Card_Live_Captions) //TODO: investigate why this only works sporatically
    {
        ESP_LOGI(TAG, "UPDATE LIVE CAPTION SCREEN");
        lv_label_set_text(ui_Card_Live_Captions_Title, title);
        lv_label_set_text(ui_Card_Live_Captions_Content, &currentCaptionString[0]);
        lv_obj_scroll_to_y(ui_Card_Live_Captions_Content, SHRT_MAX, LV_ANIM_ON);
    }
    else
    {
        ESP_LOGI(TAG, "NEW LIVE CAPTION SCREEN");
        lv_label_set_text(ui_Card_Live_Captions_Title, title);
        lv_label_set_text(ui_Card_Live_Captions_Content, &currentCaptionString[0]);
        lv_scr_load(ui_Card_Live_Captions);
        lv_obj_scroll_to_y(ui_Card_Live_Captions_Content, SHRT_MAX, LV_ANIM_OFF);
    }
    lvgl_release();
}

void displaySearchEngineResult(char* title, char* body, char* image = ""){
    updateActivity(16); //16 seconds

    lvgl_acquire();

    image = nullptr; //TODO: finish/remove this image thing
    if(image != nullptr)
    {
        //TODO: Load Card Hor or Vert depending on image resolution
        if(true) //horizontal
        {
            lv_label_set_text(ui_Label6, title);
            lv_label_set_text(ui_Label7, body);
            //TODO: set image
            lv_scr_load(ui_Card_Search_Hor);
        }
        else
        {
            //lv_obj_get_child(cui_, 0)
            lv_label_set_text(ui_Title, title);
            lv_label_set_text(ui_Content, body);
            //TODO: set image
            lv_scr_load(ui_Card_Search_Ver);
        }
    }
    else
    {
        lv_label_set_text(ui_Card_Title_Text1, title);
        lv_label_set_text(ui_CardSearchTextonlyContent, body);
        lv_scr_load(ui_Card_Search_Textonly);
    }

    lvgl_release();
}