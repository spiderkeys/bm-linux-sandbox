#pragma once

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t type;
  uint16_t checksum;
  uint8_t flags;
  uint8_t rsvd;
} __attribute__((packed)) bcmp_header_t;

typedef struct {
  // Microseconds since system has last reset/powered on.
  uint64_t time_since_boot_us;

  // How long to consider this node alive. 0 can mean indefinite, which could be useful for certain applications.
  uint32_t liveliness_lease_dur_s;
} __attribute__((packed)) bcmp_heartbeat_t;


typedef enum {
  BCMP_ACK = 0x00,
  BCMP_HEARTBEAT = 0x01,

  BCMP_ECHO_REQUEST = 0x02,
  BCMP_ECHO_REPLY = 0x03,
  BCMP_DEVICE_INFO_REQUEST = 0x04,
  BCMP_DEVICE_INFO_REPLY = 0x05,
  BCMP_PROTOCOL_CAPS_REQUEST = 0x06,
  BCMP_PROTOCOL_CAPS_REPLY = 0x07,
  BCMP_NEIGHBOR_TABLE_REQUEST = 0x08,
  BCMP_NEIGHBOR_TABLE_REPLY = 0x09,
  BCMP_RESOURCE_TABLE_REQUEST = 0x0A,
  BCMP_RESOURCE_TABLE_REPLY = 0x0B,
  BCMP_NEIGHBOR_PROTO_REQUEST = 0x0C,
  BCMP_NEIGHBOR_PROTO_REPLY = 0x0D,

  BCMP_SYSTEM_TIME_REQUEST = 0x10,
  BCMP_SYSTEM_TIME_RESPONSE = 0x11,
  BCMP_SYSTEM_TIME_SET = 0x12,

  BCMP_NET_STAT_REQUEST = 0xB0,
  BCMP_NET_STAT_REPLY = 0xB1,
  BCMP_POWER_STAT_REQUEST = 0xB2,
  BCMP_POWER_STAT_REPLY = 0xB3,

  BCMP_REBOOT_REQUEST = 0xC0,
  BCMP_REBOOT_REPLY = 0xC1,
  BCMP_NET_ASSERT_QUIET = 0xC2,

  BCMP_CONFIG_GET = 0xA0,
  BCMP_CONFIG_VALUE = 0xA1,
  BCMP_CONFIG_SET = 0xA2,
  BCMP_CONFIG_COMMIT = 0xA3,
  BCMP_CONFIG_STATUS_REQUEST = 0xA4,
  BCMP_CONFIG_STATUS_RESPONSE = 0xA5,
  BCMP_CONFIG_DELETE_REQUEST = 0xA6,
  BCMP_CONFIG_DELETE_RESPONSE = 0xA7,

  // TODO: What does DFU mean in the context of a linux device? Updating a piece of software? Updating the entire disk image? Something else?
  BCMP_DFU_START = 0xD0,
  BCMP_DFU_PAYLOAD_REQ = 0xD1,
  BCMP_DFU_PAYLOAD = 0xD2,
  BCMP_DFU_END = 0xD3,
  BCMP_DFU_ACK = 0xD4,
  BCMP_DFU_ABORT = 0xD5,
  BCMP_DFU_HEARTBEAT = 0xD6,
  BCMP_DFU_REBOOT_REQ = 0xD7,
  BCMP_DFU_REBOOT = 0xD8,
  BCMP_DFU_BOOT_COMPLETE = 0xD9,
  BCMP_DFU_LAST_MESSAGE = BCMP_DFU_BOOT_COMPLETE,
} bcmp_message_type_t;