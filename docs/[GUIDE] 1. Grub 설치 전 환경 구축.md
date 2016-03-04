--------------------------------------------------
# 1. grub 설치 전 환경 구축
--------------------------------------------------
*git 화면 위에 오른쪽에 RAW 눌러서 읽어주세요 !!*

참고 : Program 실행 시 옵션이 궁금할 경우 man [program]을 치도록 하자.
</br>
**1. Ubuntu 업데이트(매우 중요) **
```
sudo apt-get update
sudo apt-get upgrade
```
</br>
**2. 에디터 설치 (사용)**
```
sudo apt-get install vim
```
</br>
**3. git 설치 (clone 목적)**
```
sudo apt-get install git
```
</br>
**4. 소스 컴파일을 위한 기본 라이브러리 및 툴 설치**
```
sudo apt-get install autoconf automake libtool flex bison tree
```
</br>
**5. 컴파일러 설치**
```
sudo apt-get install gcc g++
```
</br>
**6. 컴파일러와 링커 최신버전 컴파일 설치를 위한 라이브러리 설치**
```
sudo apt-get install patchutils texinfo
```
</br>
**7. /usr/bin 을 최우선순위로 PATH 변수 설정(매우 중요)**
```
export PATH="/usr/bin:$PATH"
```
참고 : ./configure .... install 했을때 error gcc and g++ 관련 오류시 위에 명령어 입력하면 오류 해결가능!!
참고 2 : terminal창을 닫으면 export 옵션이 초기화 되므로 terminal 창을 껏다 켰으면 다시 실행하도록 한다.
</br>
**8. Install binutils**
</br>
&nbsp;&nbsp;&nbsp;binutils는 바이너리를 조작 하거나 바이너리 정보를 보기 위한 프로그램들의 모임으로 컴파일러에서 링커 역할을 수행하기 위해 설치 

> **Install binutils-2.25.tar.gz :**
>- http://ftp.gnu.org/gnu/binutils/에서 binutils-2.25.tar.gz(최신버전)를 다운 
>- binutils-2.25.tar.gz 압축 해제 
>- binutils-2.25 디렉토리로 이동 
>- 명령어 순차적으로 입력
```
./configure --enable-64bit-bfd --disable-shared --disable-nls
make configure-host
make all
sudo make install
```
※ 참고 : 여기서 export PATH="/usr/bin:$PATH" 작업을 하지 않으면 다음과 같은 에러가 생긴다.
```
/usr/local/bin/ld: this linker was not configured to use sysroots
collect2: error: ld returned 1 exit status
```
echo $PATH를 실행해보면 /usr/local/bin 이 /usr/bin보다 우선순위에 있기 때문이다.
</br>
**09. qemu 설치**
</br>
&nbsp;&nbsp;&nbsp;가상화 소프트웨어 가운데 하나다. Fabrice Bellard가 만들었으며 x86 이외의 기종을 위해 만들어진 소프트웨어 스택 전체를 가상머신 위에서 실행할 수 있다는 특징이 있다. 동적 변환기(Portable dynamic translation)를 사용한다.
```
sudo apt-get install qemu
```
</br>
**10. Clone SecurityBootManager Project**
```
git clone https://github.com/IWillFindYou/SecurityBootManager.git
```
</br>