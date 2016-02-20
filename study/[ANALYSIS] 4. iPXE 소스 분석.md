## iPXE-Bootloader Command Execute Call Stack 분석

순서 | 소스 경로      | 함수명        | 기능 설명
-----|----------------|---------------|------------------------------
   1 | core/main.c    | main()        | 각종 초기화 및 시작 Entry Point
     |                |               | initialise() 함수에서 명령어 등록 / startup() 함수에서 주요 프로세스들 및 장치 초기화 등록
   2 | usr/autoboot.c | ipxe()        | 초기화 완료후 shell 진입을 위한 함수 본체
   3 | hci/shell.c    | shell()       | ipxe command shell 실행 함수
     |                | shell() 내부  | while 루프 돌면서 readline_history()함수에 의해 명령라인 입력받음
   4 | core/exec.c    | system()      | 입력받은 명령라인 인자값으로 실행하는 함수
     |                | system() 내부 | 인자값으로 전달받은 char* 변수를 파싱하여 명령어 이름을 arv[0]에 저장 파라미터는 [1] ... 저장
   5 | core/exec.c    | execv()       | 첫번째 인자값의 명령어를 두번째 인자값의 파라미터로 실행
     |                |               | 예) dhcp net0 입력시 => execv("dhcp", { "net0" }); 이와 같이 호출 됨

## WPA-PSK net802.11 표준 인증 Call Stack 분석

순서 | 소스 경로                 | 함수명                      | 기능 설명
-----|---------------------------|-----------------------------|------------------------------
   1 | hci/commands/dhcp_cmd.c   | ifconf_exec()               | dhcp 명령어는 내부적으로 ifconf 명령으로 alias 되어 있음(맨아래 하단 command 구조체 참조
   2 | hci/commands/ifmgmt_cmd.c | ifconf_exec()               | ifcommon_exec() 호출하도록 되어있음 (다른 구문은 없음)
   3 | hci/commands/ifmgmt_cmd.c | ifcommon_exec()             | 
   4 | hci/commands/ifmgmt_cmd.c | ifconf_payload()            | 타겟 인터페이스가 존재할 경우, 해당 인터페이스에 등록된 payload() 호출, 아닌경우 모든 인터페이스에 대해 체크후 실행
 4-1 | core/parseopt.c           | parse_netdev()              | 타겟 인터페이스가 존재할 경우, 명령라인 파싱을 통해 인터페이스 구하여 interface 구조체에 값 할당후 리턴
   5 | usr/ifmgmt.c              | ifconf()                    | 설정한 config 값을 바탕으로 dhcp 서버 혹은 고정 아이피 할당 요청
   6 | usr/ifmgmt.c              | iflinkwait()                | 
   7 | usr/ifmgmt.c              | ifpoller_wait()             |
   8 | usr/ifmgmt.c              | monojob_wait()              | 
     |                           | monojob_wait() 내부         | 루프 돌면서 해당 인터페이스의 처리요청을 위해 등록된 칩셋에 대해 등록된 프로세스 함수 실행을 통한 처리
     | usr/ifmgmt.c              | step()                      | 프로세스 함수 실행과 더불어 가장 기초적인 스케줄링이 수행됨(라운드 로빈 / Double Linked List 이용)
     |                           | progress_descriptor::step() | 분석된 인터페이스에 해당하는 프로세스 함수 실행
     | hci/strerror.c            | step() 오류 발생시          | 해당 오류에 대한 상태번호 문자열을 리턴받아 출력
   9 | usr/ifmgmt.c              | job_progress()              | inf_get_dest_op 매크로 함수를 이용하여 도착지 인터페이스 분석을 통한 해당 기능 실행으로 보여짐
     |                           | inf_get_dest_op::desc()     | PERMANENT_PROCESS 매크로를 이용하여 함수가 등록됨
     |                           |                             | step() 함수에서 WPA-PSK 관련 기능들이 실행되어 지는 것으로 보임(조금 더 분석 필요)

