# USB Device Driver Information
해당 문서에서는 USB를 이용하여 장치를 제어하기 위한 디바이스 드라이버 개발에 관한 내용을 다룬다.

Document Author : @kbu1564

## USB 정보
이 부분에서는 각 USB 타입별 속도 및 기타 성능적인 지원되는 부분에 대한 사전 정보를 다룬다.

### 속도

speed type  | bps     | USB type     | 전송속도 | 사용 부분
------------|---------|--------------|----------|------------
super speed | 5Gbps   | USB 3.0      | 400MB/s  | 
high speed  | 480Mbps | USB 2.0      | 53MB/s   |
full speed  | 12Mbps  | USB 2.0, 1.0 | 1.2MB/s  | Serial 통신
low speed   | 1.5Mbps | USB 2.0, 1.0 | 800B/s   |

## USB 전송
- USB 를 이용하여 데이터를 전송하거나 수신을 하기 위해서는 USB 전송을 초기화 해야한다.
- 전송하게될 데이터는 그 포벳이 정확히 정해져 있다.

