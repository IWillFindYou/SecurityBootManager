# 중계 서버 UML Diagram
![serverdiagram 7](https://cloud.githubusercontent.com/assets/7445459/9430078/255ea196-4a22-11e5-9cc3-ae9e2b33d875.jpg)

기존의 Group 형식을 제거하고 자식 클라이언트 쪽의 MAC 주소를 기준으로 하여 hash_map 으로 그룹화하여 관리함<br />
부모클라이언트 : 스마트폰<br />
자식클라이언트 : PC<br />

### 중계 서버 동작 흐름
서버 구현시 Server 클래스의 method 들을 이용하여 서버를 생성하도록 되어있으며 각각의 클라이언트들은 하나의 장치로 구분되어 스마트폰 1대의 소켓에 1대의 PC 소켓이 대응 되어 처리됨<br /><br />

각각의 클라이언트에서 전송되는 데이터들은 main thread 에 의해 수신처리 되며 데이터 수신시 수신된 패킷이 어떠한 속성의 타입인지 PacketParser 클래스를 통해 decode 작업을 수행하여 PacketExecuteQueue 에 push 됨<br /><br />

push 된 큐의 Packet 데이터들을 하나씩 꺼내어 ExecuteThread 에서 처리하게되며, 해당 패킷 속성에 해당하는 execute() 함수에 의해 해당 패킷의 루틴을 수행하게 됨<br /><br />

각각의 클라이언트의 비정상 종료를 위해 특정 주기적으로 Heartbeat 를 날리는 Thread 를 하나 별도로 둠<br />
각각의 ExecuteThread 의 경우 클라이언트 단위로 패킷을 처리하도록 하는 것이 더 좋을 것 같음(Lock, UnLock 처리)

