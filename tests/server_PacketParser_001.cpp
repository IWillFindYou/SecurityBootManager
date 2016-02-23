#include "Global.h"
#include "Protocol.h"
#include "Device.h"

#include "Packet.h"
#include "PacketParser.h"
#include "packet/SetDevicePacket.h"

/**
 * Packet Structure
 * ------------------------------------------------------------------------
 * | Protocol(4byte) | Size(4byte) |           Data(Auto)                 |
 * ------------------------------------------------------------------------
 */

/**
 * 스마트폰의 서비스가 실행 되었을 때 자신이 제어할 대상을 등록 대기상태 요청
 * ----------------------------------------------------
 * | SET_DEVICE | 30 | DeviceType | 00:00:00:00:00:00 |
 * ----------------------------------------------------
 * data0 : 자신의 장치 속성값 (PHONE , PC)
 * data1 : 자신이 관리하려는 PC의 MAC 주소
 */
char buff[] = {
  0x03, 0x00, 0x00, 0x10,
  0x1e, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00,

  0x30, 0x30, 0x3a, 0x30,
  0x30, 0x3a, 0x30, 0x30,
  0x3a, 0x30, 0x30, 0x3a,
  0x30, 0x30, 0x3a, 0x30,
  0x31, 0x00
};

bool assert_func() {
  PacketParser parser;

  Protocol protocol = parser.decodeProtocol(buff);
  if (protocol != SET_DEVICE)
    return false;

  SetDevicePacket* packet = (SetDevicePacket*)parser.decode(buff, 30);
  if (packet->getDeviceType() != PC)
    return false;

  return !!(strcmp(packet->getMacAddr().c_str(), "00:00:00:00:00:00") == 0);
}

