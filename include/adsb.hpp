#pragma once
#include "all/mavlink.h"
#include "driver/uart.h"
#include "mavlink_helpers.h"
#include "mavlink_types.h"
#include "odid_wifi.h"
#include "opendroneid.h"
#include "routing.hpp"
const int32_t LATLON_MULT = 10000000;
static int intRangeMax(int64_t inValue, int startRange, int endRange) {
  if (inValue < startRange) {
    return startRange;
  } else if (inValue > endRange) {
    return endRange;
  } else {
    return (int)inValue;
  }
}
static int32_t encodeLatLon(double LatLon_data) {
  return (int32_t)intRangeMax((int64_t)(LatLon_data * LATLON_MULT), -180 * LATLON_MULT, 180 * LATLON_MULT);
}

void send_adsb(uart_port_t uart_num, uint8_t system_id, uint8_t component_id, uint8_t channel, char *callsign,
               uint32_t icao, uint32_t lat, uint32_t lon, uint8_t emitter_type, int32_t altitude, uint16_t heading,
               uint16_t hor_velocity, int16_t ver_velocity) {
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN] = {};

  int len = mavlink_msg_adsb_vehicle_pack(system_id, component_id, &msg, icao, lat, lon, ADSB_ALTITUDE_TYPE_GEOMETRIC,
                                          altitude, heading, hor_velocity, ver_velocity, callsign, emitter_type, 1,
                                          0b0010011111, 6969);
  int l = mavlink_msg_to_send_buffer(buf, &msg);
  uart_write_bytes(uart_num, buf, l);
  return;
}

void inject_adsb(ODID_UAS_Data uas_data, uart_port_t uart_num, char *own_UAS_ID) {
  if (strcmp(uas_data.BasicID[0].UASID, own_UAS_ID) == 0) {
    return;
  }
  if (uas_data.LocationValid && uas_data.BasicIDValid[0]) {
    auto lat = encodeLatLon(uas_data.Location.Latitude);
    auto lon = encodeLatLon(uas_data.Location.Longitude);
    auto heading = (uint16_t)(uas_data.Location.Direction * 100);
    auto altitude = (int32_t)(uas_data.Location.AltitudeGeo * 1000);
    char callsign[9];
    strncpy(callsign, uas_data.BasicID[0].UASID, 8); // Callsign is 8+null character in size.
    callsign[8] = '\0';
    // auto emitter_type = ADSB_EMITTER_TYPE_NO_INFO;
    auto emitter_type = ADSB_EMITTER_TYPE_UAV;
    auto horizontal_veocity = (uint16_t)(uas_data.Location.SpeedHorizontal * 100);
    auto vertical_veocity = (int16_t)(uas_data.Location.SpeedVertical * 100);
    uint32_t icao = FNV1_a_hash((uint8_t *)uas_data.BasicID[0].UASID, ODID_ID_SIZE);
    // if (uas_data.BasicID->UAType == ODID_UATYPE_HELICOPTER_OR_MULTIROTOR) {
    //   emitter_type = ADSB_EMITTER_TYPE_UAV;
    // }
    send_adsb(uart_num, 1, 156, 1, callsign, icao, lat, lon, emitter_type, altitude, heading, horizontal_veocity,
              vertical_veocity);
  }
  return;
}
