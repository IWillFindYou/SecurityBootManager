--------------------------------------------------
# 2. compile&run diskmode
--------------------------------------------------
*git 화면 위에 오른쪽에 RAW 눌러서 읽어주세요 !!*
</br>
**. /usr/bin 을 최우선순위로 PATH 변수 설정(매우 중요)**
```
export PATH="/usr/bin:$PATH"
```
참고 : ./configure .... install 했을때 error gcc and g++ 관련 오류시 위에 명령어 입력하면 오류 해결가능!!
참고 2 : terminal창을 닫으면 export 옵션이 초기화 되므로 terminal 창을 껏다 켰으면 다시 실행하도록 한다.
</br>
**1. Compile grub2**
</br>
▷ 최초로 컴파일 하는 경우
```
./linguas.sh
./autogen.sh
./configure --disable-efiemu --prefix=/home/사용자계정명/g2/usr
make
make install
```
▷ 아닌 경우
```
make
make install
```
※ 추가 설명
```
linguas.sh : 해당 국가 언어에 맞게 생성
autogen.sh : 하드웨어에 맞는 configure 파일 생성
configure : make파일을 생성, --prefix= 컴파일 된것이 생성될 경로(윈도우에서 설치시 경로 지정과 같은 것)
```
</br>
**2. Create Image file**
```
qemu-img create -f raw /home/사용자계정명/brdisk-img.raw 1G
sudo mkfs.ext2 /home/사용자계정명/brdisk-img.raw
sudo losetup /dev/loop0 /home/사용자계정명/brdisk-img.raw
```
▷ mount 해제
```
 sudo umount /dev/loop0
```
※ 추가 설명
```
[qemu-img]
Explanation : QEMU 디스크 이미지 유틸리티
Command : create [-f 디스크이미지 포멧][-o options] filename [size]
site : http://linux.die.net/man/1/qemu-img

[mkfs(Make File System)]
Explanation : 파일시스템 타입으로 포맷하여 리눅스 파일시스템을 생성, 이 가이드에서는 ext2타입으로 파일시스템 포멧
Command : mkfs [-V] [-t 파일시스템타입] [파일시스템옵션] 장치이름 [블록]

[losetup]
Explanation : loop device에 마운트, loop device는 디바이스 드라이버이다. 즉, image file이 마치 일반적인 block device인것처럼 만들어 마운트될 수 있게 하는 디바이스 드라이버이다.( 이미지 파일을 일반적인 block device 로 사용하는 것이라고 생각하면 쉽다.)

컴퓨터에서, 마운트란 파일시스템 구조 내에 있는 일련의 파일들을 사용자나 사용자 그룹들이 이용할 수 있도록 만드는 것
```
</br>
**3. Mount brdisk-img.raw**
```
mkdir /home/사용자계정명/boot
sudo mount /dev/loop0 /home/사용자계정명/boot
```
</br>
**4. Install grub2**
```
sudo ~/g2/usr/sbin/grub-install --force --no-floppy --boot-directory=/home/사용자계정명/boot /dev/loop0
```
</br>
**5. Run grub2 with qemu **
```
qemu-system-i386 -m 512 -hda /home/사용자계정명/brdisk-img.raw
```
※ 추가 설명
```
qemu는 가상화 소프트웨어 가운데 하나다. Fabrice Bellard가 만들었으며 x86 이외의 기종을 위해 만들어진 소프트웨어 스택 전체를 가상머신 위에서 실행할 수 있다는 특징이 있다. 동적 변환기(Portable dynamic translation)를 사용한다.
```