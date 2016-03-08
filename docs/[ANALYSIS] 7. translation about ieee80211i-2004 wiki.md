#ieee802.11-2004

###번역 페이지 : [링크](https://en.wikipedia.org/wiki/IEEE_802.11i-2004)

####eng

>IEEE 802.11i-2004, or 802.11i for short, is an amendment to the original IEEE 802.11, implemented as Wi-Fi Protected Access II (WPA2). The draft standard was ratified on 24 June 2004. This standard specifies security mechanisms for wireless networks, replacing the short Authentication and privacy clause of the original standard with a detailed Security clause. In the process, the amendment deprecated broken Wired Equivalent Privacy (WEP), while it was later incorporated into the published IEEE 802.11-2007 standard.

####kor

IEEE 802.11i-2004, 또는 802.11i는 WPA2로 하여 쓰이길 위한 기존 IEEE802.11의 개정판 입니다., 초고 버전은 2004년 6월 24일에 승인 되었습니다. 이 표준은 간단한 인증과 비밀 조항을 대신하여 무선 랜에서의 보안 메커니즘을 명시하고 있습니다.(개선된 버전인 것으로 보임)
여기서 개정판은 IEEE 802.11-2007 표준으로 합병된 후 좋지 못한(broken) WEP는 쓰지 않습니다.

###Replacement of WEP
####eng

>802.11i supersedes the previous security specification, Wired Equivalent Privacy (WEP), which was shown to have security vulnerabilities. Wi-Fi Protected Access (WPA) had previously been introduced by the Wi-Fi Alliance as an intermediate solution to WEP insecurities. WPA implemented a subset of a draft of 802.11i. The Wi-Fi Alliance refers to their approved, interoperable implementation of the full 802.11i as WPA2, also called RSN (Robust Security Network). 802.11i makes use of the Advanced Encryption Standard (AES) block cipher, whereas WEP and WPA use the RC4 stream cipher.[1]

####kor
802.11 기존의 보안 명세를 대신하여 (WEP는 과거에 보안 취약점을 가지고 있었음). WPA는 WEP의 보안 취약점을 대신할 녀석으로 Wi-Fi 연합을 통하여 공개 되어있었습니다. WPA는 초고802.11i 내부에 포함되어 있습니다. Wi-Fi 연합은 802.11i 안에서  상호작용 할수 있는 WPA2, RSN으로 불리는 것을 승인했다고 언급했습니다. WEP와 WPA가 RC4 stram chiper 를 쓰는데 반면 802.11i는 AES 블록 암호를 쓰도록 만들어졌습니다.

###Protocol operation
####eng
>IEEE 802.11i enhances IEEE 802.11-1999 by providing a Robust Security Network (RSN) with two new protocols, the 4-Way Handshake and the Group Key Handshake. These utilize the authentication services and port access control described in IEEE 802.1X to establish and change the appropriate cryptographic keys.[2][3] The RSN is a security network that only allows the creation of robust security network associations (RSNAs), which are a type of association used by a pair of stations (STAs) if the procedure to establish authentication or association between them includes the 4-Way Handshake.[4]
>The standard also provides two RSNA data confidentiality and integrity protocols, TKIP and CCMP, with implementation of CCMP being mandatory.[5]
>The initial authentication process is carried out either using a pre-shared key (PSK), or following an EAP exchange through 802.1X (known as EAPOL, which requires the presence of an authentication server). This process ensures that the client station (STA) is authenticated with the access point (AP). After the PSK or 802.1X authentication, a shared secret key is generated, called the Pairwise Master Key (PMK). To derive the PMK from the PSK, the PSK is put through PBKDF2-SHA1 as the cryptographic hash function. If an 802.1X EAP exchange was carried out, the PMK is derived from the EAP parameters provided by the authentication server.

####kor
IEEE 802.11i는 802.11-1999로 부터 RSN의 새로운 두 Protocols인 4-way handshake 와 group key handshake를 제공하도록 개선되었습니다.
이 인증 기능 들과 접근 제어는 암호키에 적합하도록 개정되고 바뀌어져서 IEEE 802.1X에 명시되어 있습니다. RSN은 오로지 RSNA에 따르는 보안 네트워크입니다. RSN는 기기간 4-Way handshake를 통하여 인증 또는 연결 절차가 수립될때의 인증 과정입니다. 그 표준은 물론 RSNA 데이터 기밀성 과 무결성 프로토콜 또한 제공합니다. 바로 TKIP와 CCMP로 불려지며 이 중 CCMP는 필수적으로 필수로 사용되고 있습니다.
인증의 시작은 PSK를 이용하거나 802.1X를 통한 EAP 교환(EAPOL이라고 불려지며 인증 서버를 필요로 합니다.)을 이용하는 것입니다. 이 과정은 client가 AP와 인증되도록 보장합니다. PSK인증 또는 802.1X인증 이후 공유 보안 키는 생성되고, 이것은 PMK(Pairwise Master Key)로 불립니다. PSK로 부터 PMK를 가져오기 위해서 PSK는 암호 해쉬 함수인 PMKDF2-SHA1를 거쳐야 합니다. 만약 802.1X EAP 교환 방식(802.1X인증)으로 수행된다면 PMK는 인증 서버를 통하여 EAP 매개 변수들로 부터 가져와야 합니다.

d
