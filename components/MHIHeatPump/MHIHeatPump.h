#pragma once

#include "MHI-AC-Ctrl-core.h"

#include "esphome/components/climate/climate.h"

#include "esphome/components/sensor/sensor.h"

#define TROOM_FILTER_LIMIT 0.25

using namespace esphome;
using namespace esphome::climate;

class MhiHeatPump 
     : public Component 
{
public:
  void setup() override {
  }

  void loop() override {
  }
};

class MHIHeatPump 
     : public Component 
     , public climate::Climate
     , public CallbackInterface_Status
{
public:

  void setup() override {
    m_ac.MHIAcCtrlStatus(this);
    m_ac.init();
  }

  void loop() override {
    m_ac.loop(25);
  }

static optional<ClimateFanMode> fanToClimate(int mode)
{
     switch(mode)
     {
          case 7: return CLIMATE_FAN_AUTO;
          case 0: return CLIMATE_FAN_DIFFUSE;
          case 1: return CLIMATE_FAN_LOW;
          case 2: return CLIMATE_FAN_MEDIUM;
          case 6: return CLIMATE_FAN_HIGH;
     }
     return {};
}

static optional<int> climateToFan(ClimateFanMode fanMode)
{
     switch (fanMode)
     {
          case CLIMATE_FAN_AUTO: return 7;
          case CLIMATE_FAN_DIFFUSE: return 0;
          case CLIMATE_FAN_LOW: return 1;
          case CLIMATE_FAN_MEDIUM: return 2;
          case CLIMATE_FAN_HIGH: return 6;
          default: return {};
     }
}       
  
   void control(const ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      // User requested mode change
      ClimateMode mode = *call.get_mode();
      // Send mode to hardware
      // ...
      if (mode == CLIMATE_MODE_OFF) {
        m_ac.set_power(false);
        m_on = false;
      } else if (mode == CLIMATE_MODE_HEAT) {
        m_mode = CLIMATE_MODE_HEAT;
        m_on = true;
        m_ac.set_mode(ACMode::mode_heat);
        m_ac.set_power(true);
      } else if (mode == CLIMATE_MODE_COOL) {
        m_mode = CLIMATE_MODE_COOL;
        m_on = true;
        m_ac.set_mode(ACMode::mode_cool);
        m_ac.set_power(true);
      } else if (mode == CLIMATE_MODE_FAN_ONLY) {
        m_mode = CLIMATE_MODE_FAN_ONLY;
        m_on = true;
        m_ac.set_mode(ACMode::mode_fan);
        m_ac.set_power(true);
      }
    
      // Publish updated state
      this->mode = mode;
      this->publish_state();
    }
    if (call.get_target_temperature().has_value()) {
      // User requested target temperature change
      float temp = *call.get_target_temperature() + m_ac.get_troom_offset();
      m_ac.set_tsetpoint((byte)temp * 2);
      // Send target temp to climate
      // ...
      this->target_temperature = call.get_target_temperature().value();
      this->publish_state();
    }
    if (call.get_fan_mode().has_value()) {
         auto val  = climateToFan(call.get_fan_mode().value());
         if (val)
         {
              m_ac.set_fan(*val);
              this->fan_mode = call.get_fan_mode().value();
              this->publish_state();
         }
    }
  }

  ClimateTraits traits() override {
    // The capabilities of the climate device
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supported_modes({CLIMATE_MODE_OFF, CLIMATE_MODE_COOL, CLIMATE_MODE_HEAT, CLIMATE_MODE_FAN_ONLY});
    //traits.set_supports_auto_mode(false);
    //traits.set_supports_heat_mode(true);
    //traits.set_supports_cool_mode(true);
    traits.set_visual_temperature_step(0.5f);
    std::set<esphome::climate::ClimateFanMode> fan_modes = { CLIMATE_FAN_DIFFUSE, CLIMATE_FAN_LOW, CLIMATE_FAN_MEDIUM, CLIMATE_FAN_HIGH, CLIMATE_FAN_AUTO };
    traits.set_supported_fan_modes(fan_modes);
    //traits.set_supports_fan_mode_low(true);
    //traits.set_supports_fan_mode_medium(true);
    //traits.set_supports_fan_mode_high(true);
    //traits.set_supports_fan_mode_auto(true);
    
    return traits;
  }
  
  void cbiStatusFunction(ACStatus status, int value) override
  {
     float offset = m_ac.get_troom_offset();
     float tmp_value;
     static byte status_troom_old=0xff;
     switch(status)
     {
        case status_mode:
        case opdata_mode: 
             ESP_LOGI("custom", "Got status callback opdata_mode %x (%d0", value, status);
             {
                ClimateMode mode = CLIMATE_MODE_COOL;
                if (value == mode_heat) {
                    m_mode = CLIMATE_MODE_HEAT;
                } else if (value == mode_cool) {
                    m_mode = CLIMATE_MODE_COOL;
                }
                if (m_on) {
                    this->mode = m_mode;
                    publish_state();    
                }
             }
             break;
        case opdata_return_air: 
             ESP_LOGI("custom", "Got opdata_return_air temp %f", (value - 61) / 4.0);
             this->current_temperature = (value - 61) / 4.0;
             publish_state();
             break;
        case status_troom:
          {
            int8_t troom_diff = value - status_troom_old; // avoid using other functions inside the brackets of abs, see https://www.arduino.cc/reference/en/language/functions/math/abs/
            if (abs(troom_diff) > TROOM_FILTER_LIMIT/0.25f) { // Room temperature delta > 0.25°C
              status_troom_old = value;
            }
          }
             ESP_LOGI("custom", "Got opdata_return_air temp %f", (value - 61) / 4.0);
             break;
        case opdata_outdoor: 
             {
             const float temp = (value - 94) * 0.25f;
             ESP_LOGI("custom", "Got outdoor temp %f", temp);
             if (this->outdoor_temperature_)
                this->outdoor_temperature_->publish_state(temp);
             break;
             }
        case opdata_iu_fanspeed: 
             ESP_LOGI("custom", "Got indoor fan speed %d", value);
             break;
        case opdata_ou_fanspeed: 
             ESP_LOGI("custom", "Got outdoor fan speed %d", value);
             break;
        case opdata_total_comp_run: 
             ESP_LOGI("custom", "Got opdata_total_comp_run %d", value);
             break;
        case status_power:
             ESP_LOGI("custom", "Got power %d", value);
             {
                if (value == 0) {
                    m_on = false;
                    this->mode = CLIMATE_MODE_OFF;
                    publish_state();    
                } else {
                    m_on = true;
                    this->mode = m_mode;
                    publish_state();
                }
             }
             break;
        
        case opdata_thi_r1:
             ESP_LOGI("custom", "Got thi1 %d", (0.327f * value - 11.4f));
             break;
        case opdata_thi_r2:
             ESP_LOGI("custom", "Got thi2 %d", (0.327f * value - 11.4f));
             break;
        case opdata_thi_r3:
             ESP_LOGI("custom", "Got thi3 %d", (0.327f * value - 11.4f));
             break;
        case opdata_tho_r1:
             ESP_LOGI("custom", "Got tho-r1 %d", (0.327f * value - 11.4f));
             break;
        case opdata_total_iu_run:
             ESP_LOGI("custom", "Got opdata_total_iu_run %d", value * 100);
             break;
        case opdata_ou_eev1:
             ESP_LOGI("custom", "Got v %d %d", value, opdata_ou_eev1);
             break;
        case opdata_comp:
             ESP_LOGI("custom", "Got opdata_comp %f", highByte(value) * 25.6f + 0.1f * lowByte(value));
             break;
        case opdata_ct:
               {
               const double amps = value * 14 / 51.0f;
             ESP_LOGI("custom", "Got opdata_ct %f %f", value * 14 / 51.0f, amps * 240);
             if (this->current_)
               this->current_->publish_state(amps);
             break;
               }
        case opdata_tsetpoint:
          ESP_LOGI("custom", "Got opdata_tsetpoint %d (%d)", value, status);
          break;
        case status_tsetpoint:
               ESP_LOGI("custom", "Got status_tsetpoint %d (%d)", value, status);
               tmp_value = (value & 0x7f)/ 2.0;
               offset = round(tmp_value) - tmp_value;  // Calculate offset when setpoint is changed
               m_ac.set_troom_offset(offset);
               this->target_temperature = (value & 0x7f)/ 2.0;
               publish_state();
               break;
        case status_vanes:
               ESP_LOGI("custom", "Got status_vanes %d", value);
               break;
        case opdata_tdsh:
               ESP_LOGI("custom", "Got opdata_tdsh %d", value);
               break;
     case opdata_td:
               ESP_LOGI("custom", "Got Discharge Pipe Temperature %d", value);
               break;
     case opdata_defrost:
     
               ESP_LOGI("custom", "Got opdata_defrost %d", value);
               break;
     case opdata_protection_no:
               
               ESP_LOGI("custom", "Got opdata_protection_no %d", value);
               break;
     case status_fan:
               ESP_LOGI("custom", "Got status_fan %d", value);
               {
                    auto fan = fanToClimate(value);
                    if (fan)
                    {
                         this->fan_mode = fan;
                         publish_state();
                    }
               }
               break;
    //opdata_mode = type_opdata, opdata_tsetpoint, opdata_return_air, opdata_outdoor, opdata_tho_r1, opdata_iu_fanspeed, opdata_thi_r1, opdata_thi_r2, opdata_thi_r3,
  //opdata_ou_fanspeed, opdata_total_iu_run, opdata_total_comp_run, opdata_comp, opdata_ct, opdata_td,
  //opdata_tdsh, opdata_protection_no, opdata_defrost, opdata_ou_eev1, opdata_unknwon,
  
        default:
          ESP_LOGI("custom", "Got status callback %d %d", status, value);
          break;
    }
  }

  void set_outdoor_temperature_sensor(sensor::Sensor *outdoor_temperature) { outdoor_temperature_ = outdoor_temperature; }
  void set_current_sensor(sensor::Sensor *current) { current_ = current; }
  MHI_AC_Ctrl_Core m_ac;
  bool m_on;
  ClimateMode m_mode;

  sensor::Sensor* error_code_;
  sensor::Sensor* outdoor_temperature_;
  sensor::Sensor* return_air_temperature_;
  sensor::Sensor* outdoor_unit_fan_speed_;
  sensor::Sensor* indoor_unit_fan_speed_;
  sensor::Sensor* compressor_frequency_;
  sensor::Sensor* indoor_unit_total_run_time_;
  sensor::Sensor* compressor_total_run_time_;
  sensor::Sensor* current_;
  //sensor::BinarySensor* defrost_;
};