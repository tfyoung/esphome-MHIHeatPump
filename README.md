# esphome-MHIHeatPump
Component for esphome to use spi to connect to a Mitsubishi Heavy Industries heatpump/AC

# Authors
Most of this code was written by others.

https://github.com/absalom-muc/MHI-AC-Ctrl - This is the bulk of the protocol and also has hardware notes
https://github.com/ginkage/MHI-AC-Ctrl-ESPHome - Did a lot of integration work with esphome, some of which i borrowed
https://github.com/geoffdavis/esphome-mitsubishiheatpump - Did all the nice component work for a heatpump which was borrowed

## ESPHOME Config
The applicable config for the device should look something like (after the wifi and board config):

```yaml
external_components:
  - source: github://tfyoung/esphome-MHIHeatPump

substitutions:
  name: acloungemhi
  friendly_name: Lounge AC
  device_id: mhiac_lounge

time:
  - platform: homeassistant
    id: homeassistant_time

MHIHeatPump:
  id: ${device_id}

climate:
  - platform: MHIHeatPump
    id: lounge
    name: "${friendly_name}"
    mhi_heatpump_id: ${device_id}

sensor:
  - platform: MHIHeatPump
    mhi_heatpump_id: ${device_id}
    outdoor_temperature:
      name: "Outdoor Temperature"
    current:
      id: current
      name: "Current"
  - platform: template
    name: ${friendly_name} power
    id: "power"
    lambda: return id(current).state * 230;  #I think all units are 230V; real voltage can vary a bit, but has a small impact.
    unit_of_measurement: W
    device_class: energy
  - platform: total_daily_energy
    name: ${friendly_name} daily energy
    power_id: "power"
    unit_of_measurement: Wh
    device_class: energy

```

