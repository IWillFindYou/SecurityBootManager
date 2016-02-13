## iPXE-Bootloader Command Execute Call Stack 분석
##### 참고 : [iPXE documents](http://dox.ipxe.org/files.html)

순서 | 소스 경로      | 함수명        | 기능 설명
-----|----------------|---------------|------------------------------
   1 | [core/main.c](http://dox.ipxe.org/main_8c.html)    | [main()](http://dox.ipxe.org/stdlib_8h.html#a8f9c7ee03b93bbfcc6355f694f218c4d)        | 각종 초기화 및 시작 Entry Point
     |                |               | initialise() 함수에서 명령어 등록 / startup() 함수에서 주요 프로세스들 및 장치 초기화 등록
   2 | [usr/autoboot.c](http://dox.ipxe.org/autoboot_8h.html) | [ipxe()](http://dox.ipxe.org/autoboot_8h.html#a877ee227403a5b5b29c9831d7b8e3d0f)        | 초기화 완료후 shell 진입을 위한 함수 본체
   3 | [hci/shell.c](http://dox.ipxe.org/shell_8c.html)    | [shell()](http://dox.ipxe.org/shell_8c.html#a225cd0af1ed8359b2973453e2a22a94b)       | ipxe command shell 실행 함수
     |                | shell() 내부  | while 루프 돌면서 readline_history()함수에 의해 명령라인 입력받음
   4 | [core/exec.c](http://dox.ipxe.org/exec_8c.html)    | [system()](http://dox.ipxe.org/exec_8c.html#a0680e05faf0347c931689fb3fe3d7403)      | 입력받은 명령라인 인자값으로 실행하는 함수
     |                | system() 내부 | 인자값으로 전달받은 char* 변수를 파싱하여 명령어 이름을 arv[0]에 저장 파라미터는 [1] ... 저장
   5 | [core/exec.c](http://dox.ipxe.org/exec_8c.html)    | [execv()](http://dox.ipxe.org/exec_8c.html#a27bd5952b21686f0c85fc9128d117b1a)       | 첫번째 인자값의 명령어를 두번째 인자값의 파라미터로 실행
     |                |               | 예) dhcp net0 입력시 => execv("dhcp", { "net0" }); 이와 같이 호출 됨

## WPA-PSK net802.11 표준 인증 Call Stack 분석

순서 | 소스 경로                 | 함수명                      | 기능 설명
-----|---------------------------|-----------------------------|------------------------------
   1 | [hci/commands/dhcp_cmd.c](http://dox.ipxe.org/dhcp__cmd_8c.html)   | [ifconf_exec()](http://dox.ipxe.org/ifmgmt__cmd_8c.html#a89441d3de3b2530b35770925ed12f7f4)               | dhcp 명령어는 내부적으로 ifconf 명령으로 alias 되어 있음(맨아래 하단 command 구조체 참조
   2 | [hci/commands/ifmgmt_cmd.c](http://dox.ipxe.org/ifmgmt__cmd_8c.html) | [ifconf_exec()](http://dox.ipxe.org/ifmgmt__cmd_8c.html#a89441d3de3b2530b35770925ed12f7f4)               | ifcommon_exec() 호출하도록 되어있음 (다른 구문은 없음)
   3 | [hci/commands/ifmgmt_cmd.c](http://dox.ipxe.org/ifmgmt__cmd_8c.html) | [ifcommon_exec()](http://dox.ipxe.org/ifmgmt__cmd_8c.html#a0f495cb2d81aa098ac4ee985287309e2)             | 
   4 | [hci/commands/ifmgmt_cmd.c](http://dox.ipxe.org/ifmgmt__cmd_8c.html) | [ifconf_payload()](http://dox.ipxe.org/ifmgmt__cmd_8c.html#ada754bfbac175aa1d3d8d66f359e23a8)            | 타겟 인터페이스가 존재할 경우, 해당 인터페이스에 등록된 payload() 호출, 아닌경우 모든 인터페이스에 대해 체크후 실행
 4-1 | [core/parseopt.c](http://dox.ipxe.org/parseopt_8c.html)           | [parse_netdev()](http://dox.ipxe.org/parseopt_8c.html#a11d157d0406e999be3b900303df1453b)              | 타겟 인터페이스가 존재할 경우, 명령라인 파싱을 통해 인터페이스 구하여 interface 구조체에 값 할당후 리턴
   5 | [usr/ifmgmt.c](http://dox.ipxe.org/ifmgmt_8c.html)              | [ifconf()](http://dox.ipxe.org/ifmgmt_8c.html#a0932f1fa2c5a7fe0655a96acf3841118)                    | 설정한 config 값을 바탕으로 dhcp 서버 혹은 고정 아이피 할당 요청
   6 | [usr/ifmgmt.c](http://dox.ipxe.org/ifmgmt_8c.html)             | [iflinkwait()](http://dox.ipxe.org/ifmgmt_8c.html#a48bad5ff6ac19ea958244db31309b427)                | 
   7 | [usr/ifmgmt.c](http://dox.ipxe.org/ifmgmt_8c.html)             | [ifpoller_wait()](http://dox.ipxe.org/ifmgmt_8c.html#ab8b578ccee4c15094ae0f4b024220f9b)             |
   8 | [core/monojob.c](http://dox.ipxe.org/monojob_8c.html)             | [monojob_wait()](http://dox.ipxe.org/monojob_8c.html#ab4226874dd401b29b18afc7703adc3f9)              | 
     |                           | monojob_wait() 내부         | 루프 돌면서 해당 인터페이스의 처리요청을 위해 등록된 칩셋에 대해 등록된 프로세스 함수 실행을 통한 처리
     | [core/process.c](http://dox.ipxe.org/process_8c.html)            | [step()](http://dox.ipxe.org/process_8c.html#a9f1dffc0dde87c0cc5eea9fc77d89b28)                      | 프로세스 함수 실행과 더불어 가장 기초적인 스케줄링이 수행됨(라운드 로빈 / Double Linked List 이용)
     |                           | progress_descriptor::step() | 분석된 인터페이스에 해당하는 프로세스 함수 실행
     | [hci/strerror.c](http://dox.ipxe.org/strerror_8c.html)            | step() 오류 발생시          | 해당 오류에 대한 상태번호 문자열을 리턴받아 출력
   9 | [core/job.c](http://dox.ipxe.org/job_8c.html)             | [job_progress()](http://dox.ipxe.org/job_8c.html#a96c6fc34d7155d47c1c61ffa7fb3c0f9)              | inf_get_dest_op 매크로 함수를 이용하여 도착지 인터페이스 분석을 통한 해당 기능 실행으로 보여짐
     |                           | [intf_get_dest_op::desc()](http://dox.ipxe.org/interface_8h.html#a0fafe798f9cdbe389bd46d25d7a3e8da)     | PERMANENT_PROCESS 매크로를 이용하여 함수가 등록됨
     |                           |                             | step() 함수에서 WPA-PSK 관련 기능들이 실행되어 지는 것으로 보임(조금 더 분석 필요)
