package com.example.testclient;

import android.app.Activity;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteOrder;

public class MainActivity extends Activity {

    private TextView txtResponse;
    private EditText edtTextAddress, edtTextPort;
    private Button btnConnect, btnClear;

    private Socket socket = null;
    private DataInputStream dis = null;
    private DataOutputStream dos = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        edtTextAddress = (EditText) findViewById(R.id.address);
        edtTextPort = (EditText) findViewById(R.id.port);
        btnConnect = (Button) findViewById(R.id.connect);
        btnClear = (Button) findViewById(R.id.clear);
        txtResponse = (TextView) findViewById(R.id.response);

        btnConnect.setOnClickListener(buttonConnectOnClickListener);
        btnClear.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                txtResponse.setText("");
                try {
                    socket.close();
                } catch (IOException e) {
                    Log.e("Error", e.getMessage());
                }
            }
        });
    }

    // 클릭이벤트 리스너
    OnClickListener buttonConnectOnClickListener = new OnClickListener() {

        public void onClick(View arg0) {
            NetworkTask myClientTask = new NetworkTask(
                    edtTextAddress.getText().toString(),
                    Integer.parseInt(edtTextPort.getText().toString())
            );
            myClientTask.execute();
        }
    };

    /**
     * @FileName        : MainActivity.java
     * @Project        : NetworkTask
     * @Date            : 2015. 8. 23.
     * @작성자            : 주현
     * @프로그램 설명        : 비동기 쓰레드 네트워크 통신
     * @프로그램 기능        : 중개서버와 TCP/IP 소켓 통신
     * @변경이력        :
     */
    public class NetworkTask extends AsyncTask<Void, Void, Void> {

        String dstAddress;
        int dstPort;
        String response;

        // 소켓 연결 시도 수
        int connCount;

        NetworkTask(String addr, int port) {
            dstAddress = addr;
            dstPort = port;

            connCount = 0;
        }

        /**
         * @Method Name    :  doInBackground
         * @Method 기능    :  백그라운드에서 중개 서버와 TCP/IP 소켓 통신
         * @변경이력        :
         */
        @Override
        protected Void doInBackground(Void... arg0) {

            // 중개 서버와 TCP/IP 소켓 통신
            network();

            return null;
        }

        /**
         * @Method Name    :  getbytes
         * @Method 기능    :  byte를 지정한 offset만큼 얻는 함수
         * @변경이력        :
         */
        private byte[] getbytes(byte src[], int offset, int length) {
            byte dest[] = new byte[length];
            System.arraycopy(src, offset, dest, 0, length);
            return dest;
        }

        /**
         * @Method Name    :  network
         * @Method 기능    :  중개서버와 TCP/IP 소켓 통신
         * @변경이력        :
         */
        private void network() {
            try {

                socket = new Socket(dstAddress, dstPort);

                // 정상적으로 소켓이 연결되었을 경우
                if (socket != null)
                    connection();

            } catch (Exception e) {
                if (connCount < 5)
                    network();
                else
                    connCount = 0;
            }
        }

        /**
         * @Method Name    :  connection
         * @Method 기능    :  정상적으로 소켓이 연결된 경우 중개 서버로부터 메시지 수신
         * @변경이력        :
         */
        private void connection() {
            try {
                dis = new DataInputStream(socket.getInputStream());
                dos = new DataOutputStream(socket.getOutputStream());

            } catch (UnknownHostException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }

            sendMessage(Protocol.SET_DEVICE);


            while (true) {
                try {
                    byte[] msg = new byte[1024];

                    // 메지시를 읽어 바이트 배열에 저장
                    dis.read(msg, 0, msg.length);


                    // 받은 메시지를 처리 한다.
                    receiveMsg(msg);
                } catch (IOException e1) {
                    if (dos != null) try {
                        dos.close();
                    } catch (IOException e2) {
                        Log.e("ERROR::connection()", e2.getMessage());
                    }
                    if (dis != null) try {
                        dos.close();
                    } catch (IOException e2) {
                        Log.e("ERROR::connection()", e2.getMessage());
                    }
                    if (socket != null) try {
                        dos.close();
                    } catch (IOException e2) {
                        Log.e("ERROR::connection()", e2.getMessage());
                    }
                    break;
                }
            }

        }

        /**
         * @Method Name    :  receiveMsg
         * @Method 기능    :  수신 메시지를 프로토콜 규약에 맞게 처리
         * @변경이력        :
         */
        private void receiveMsg(byte[] message) {
            int protocol = ByteType.byteToInt(getbytes(message, 0, 4), ByteOrder.LITTLE_ENDIAN);
            int size = ByteType.byteToInt(getbytes(message, 4, 4), ByteOrder.LITTLE_ENDIAN);
            String data = null;

            switch (protocol) {

                // HeartBeat 체크용 프로토콜
                case Protocol.PING_DEVICE:

                    // 프로세스가 살아 있음을 메시지로 보낸다.
                    sendMessage(Protocol.PONG_DEVICE);

                    break;

                // 부팅상태값 요청
                case Protocol.BOOTING_REQUEST:
                    // 앱 실행!

                    break;
            }

            StringBuffer sb = new StringBuffer();
            sb.append("protocol(4bytes):" + protocol);
            sb.append("\nSize(4bytes):" + size);
            sb.append("\nData(Auto):" + data);

            response = sb.toString();

        }

        /**
         * @Method Name    :  receiveMsg
         * @Method 기능    :  프로토콜 규약에 메시지 송신
         * @변경이력        :
         */
        private void sendMessage(int protocol) {
            byte[] sendMsg = null;

            String pcMac = "52:00:00:00:00:01";
            int deviceType = DeviceType.PHONE;

            try {

                switch (protocol) {

                    // HeartBeat 응답에 대한 프로토콜
                    case Protocol.PONG_DEVICE:

                        sendMsg = makeMsg(protocol, 0xFFFF);
                        dos.write(sendMsg);

                        break;

                    // 스마트폰의 서비스가 실행 되었을 때 자신이 제어할 대상을 등록 대기상태 요청
                    case Protocol.SET_DEVICE:

                        sendMsg = makeMsg(protocol, deviceType, pcMac);
                        dos.write(sendMsg);

                        break;

                    case Protocol.SHUTDOWN_DEVICE:          // Phone => Grub : 장치 강제 종료
                    case Protocol.BOOTING_DEVICE:           // Phone => Grub : 장치 부팅 진행

                        sendMsg = makeMsg(protocol, pcMac);
                        dos.write(sendMsg);

                        break;
                }
            } catch (IOException e) {
                Log.e("ERROR:sendMessage()", "Fail send message");
            }
        }

        /**
         * @Method Name    : makeMsg
         * @Method 기능    : 송신을 위해 메시지의 데이터 타입을 byte 타입으로 변환
         * @변경이력        :
         */
        private byte[] makeMsg(int protocol, int data) {

            int buffSize = 8 + Integer.SIZE / 8;

            // byte 변환
            byte[] sendProtocol = ByteType.intToByte(protocol, ByteOrder.LITTLE_ENDIAN);
            byte[] sendSize = ByteType.intToByte(buffSize, ByteOrder.LITTLE_ENDIAN);
            byte[] sendData = ByteType.intToByte(data, ByteOrder.LITTLE_ENDIAN);

            // byte 합치기
            byte[] sendMsg = new byte[buffSize];
            System.arraycopy(sendProtocol, 0, sendMsg, 0, sendProtocol.length);
            System.arraycopy(sendSize, 0, sendMsg, 4, sendSize.length);
            System.arraycopy(sendData, 0, sendMsg, 8, sendData.length);

            return sendMsg;
        }

        private byte[] makeMsg(int protocol, String data) {

            int buffSize = 8 + data.length();

            // byte 변환
            byte[] sendProtocol = ByteType.intToByte(protocol, ByteOrder.LITTLE_ENDIAN);
            byte[] sendSize = ByteType.intToByte(buffSize, ByteOrder.LITTLE_ENDIAN);
            byte[] sendData = ByteType.stringToByte(data, ByteOrder.LITTLE_ENDIAN);

            // byte 합치기
            byte[] sendMsg = new byte[buffSize];
            System.arraycopy(sendProtocol, 0, sendMsg, 0, sendProtocol.length);
            System.arraycopy(sendSize, 0, sendMsg, 4, sendSize.length);
            System.arraycopy(sendData, 0, sendMsg, 8, sendData.length);

            return sendMsg;
        }

        private byte[] makeMsg(int protocol, int deviceType, String data) {

            int buffSize = 8 + Integer.SIZE / 8 + data.length();

            // byte 변환
            byte[] sendProtocol = ByteType.intToByte(protocol, ByteOrder.LITTLE_ENDIAN);
            byte[] sendSize = ByteType.intToByte(buffSize, ByteOrder.LITTLE_ENDIAN);
            byte[] sendDeviceType = ByteType.intToByte(deviceType, ByteOrder.LITTLE_ENDIAN);
            byte[] sendData = ByteType.stringToByte(data, ByteOrder.LITTLE_ENDIAN);

            // byte 합치기
            byte[] sendMsg = new byte[buffSize];
            System.arraycopy(sendProtocol, 0, sendMsg, 0, sendProtocol.length);
            System.arraycopy(sendSize, 0, sendMsg, 4, sendSize.length);
            System.arraycopy(sendDeviceType, 0, sendMsg, 8, sendDeviceType.length);
            System.arraycopy(sendData, 0, sendMsg, 12, sendData.length);

            return sendMsg;
        }

        /**
         * @Method Name    : onPostExecute
         * @Method 기능    : 모든 실행이 완료된 후 처리
         * @변경이력        :
         */
        @Override
        protected void onPostExecute(Void result) {
            if (dos != null) try {
                dos.close();
            } catch (IOException e2) {
                Log.e("ERROR::connection()", e2.getMessage());
            }
            if (dis != null) try {
                dos.close();
            } catch (IOException e2) {
                Log.e("ERROR::connection()", e2.getMessage());
            }
            if (socket != null) try {
                dos.close();
            } catch (IOException e2) {
                Log.e("ERROR::connection()", e2.getMessage());
            }

            super.onPostExecute(result);
        }

    }

}