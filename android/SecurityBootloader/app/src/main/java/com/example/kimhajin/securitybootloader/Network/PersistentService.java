package com.example.kimhajin.securitybootloader.Network;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.IBinder;
import android.os.SystemClock;
import android.util.Log;
import android.widget.Toast;

import com.example.kimhajin.securitybootloader.MainActivity;
import com.example.kimhajin.securitybootloader.R;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteOrder;

/** 
 * @FileName		: PersistentService.java 
 * @Project		: SecurityBootloder 
 * @Date			: 2015. 8. 24. 
 * @작성자			: 주현 
 * @프로그램 설명		: 백그라운드에서 계속 중개서버와 통신을 하는 클레스
 * @프로그램 기능		: 
 * @변경이력		: 
 */

public class PersistentService extends Service implements Runnable {

    private static final String TAG = "PersistentService";

    // 서비스 종료시 재부팅 딜레이 시간, activity의 활성 시간을 벌어야 한다.
    private static final int REBOOT_DELAY_TIMER = 10 * 1000;

    // 서비스 실행 시간
    private static final int LOCATION_UPDATE_DELAY = 3 * 1000;

    private Handler mHandler;
    private boolean mIsRunning;

    // TCP/IP
    private Socket socket = null;
    private DataInputStream dis = null;
    private DataOutputStream dos = null;

    private NetworkTask myClientTask = null;

    @Override
    public IBinder onBind(Intent intent) {
        Log.d("PersistentService", "onBind()");
        return null;
    }

    /**
     * @Method Name    :  onCreate
     * @Method 기능    :  등록된 알람 제거
     * @변경이력        :
     */
    @Override
    public void onCreate() {
        Log.d("PersistentService", "onCreate()");
        unregisterRestartAlarm();
        super.onCreate();
        mIsRunning = false;

    }

    /**
     * @Method Name    :  onDestroy
     * @Method 기능    :  서비스가 죽었을때 알람 등록
     * @변경이력        :
     */
    @Override
    public void onDestroy() {
        Log.d("PersistentService", "onDestroy()");
        registerRestartAlarm();
        super.onDestroy();
        mIsRunning = false;
    }

    /**
     * @Method Name    :  onStart
     * @Method 기능    :  서비스가 시작되었을때 run()이 실행되기까지 delay를 handler를 통해서 주고 있다.
     * @변경이력        :
     */
    @Override
    public void onStart(Intent intent, int startId) {

        Log.d("PersistentService", "onStart()");
        super.onStart(intent, startId);

        mHandler = new Handler();
        mHandler.postDelayed(this, LOCATION_UPDATE_DELAY);
        mIsRunning = true;
    }

    /**
     * @Method Name    :  run
     * @Method 기능    :  서비스가 돌아가고 있을때 실제로 내가 원하는 기능을 구현하는 부분
     * @변경이력        :
     */
    @Override
    public void run() {

        Log.e(TAG, "run()");

        if(!mIsRunning)
        {
            Log.d("PersistentService", "run(), mIsRunning is false");
            Log.d("PersistentService", "run(), alarm service end");

            return;
        } else {
            if(socket == null || !socket.isConnected()) {

                Log.d("PersistentService", "run(), mIsRunning is true");
                Log.d("PersistentService", "run(), alarm repeat after five minutes");

                Toast.makeText(getApplicationContext(), "network start", Toast.LENGTH_SHORT).show();
                myClientTask = new NetworkTask("222.122.81.58", 10888);
                myClientTask.execute();
                Toast.makeText(getApplicationContext(), "network finish", Toast.LENGTH_SHORT).show();

                mHandler.postDelayed(this, LOCATION_UPDATE_DELAY);
                mIsRunning = true;
            }
        }
    }

    /**
     * @Method Name    :  onStartCommand
     * @Method 기능    :  버튼 클릭에 따라 중개서버에 적절한 메시지를 송신
     * @변경이력        :
     */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        boolean isStartBtn = intent.getBooleanExtra("startBtn", false);
        boolean isCloseBtn = intent.getBooleanExtra("cancelBtn", false);

        if(isStartBtn){

            myClientTask.sendMessage(Protocol.BOOTING_DEVICE);
            //Toast.makeText(getApplicationContext(), "startBtn", Toast.LENGTH_LONG).show();

            isStartBtn = false;
        }
        if(isCloseBtn){

            myClientTask.sendMessage(Protocol.SHUTDOWN_DEVICE);

            //Toast.makeText(getApplicationContext(), "cancelBtn", Toast.LENGTH_LONG).show();

            isCloseBtn = false;
        }

        return super.onStartCommand(intent, flags, startId);
    }

    /**
     * @Method Name    :  unregisterRestartAlarm
     * @Method 기능    :  서비스가 시스템에 의해서 또는 강제적으로 종료되었을 때 호출되어
     *                      알람을 등록해서 10초 후에 서비스가 실행되도록 한다.
     * @변경이력        :
     */
    private void registerRestartAlarm() {

        Log.d("PersistentService", "registerRestartAlarm()");

        Intent intent = new Intent(PersistentService.this, RestartService.class);
        intent.setAction(RestartService.ACTION_RESTART_PERSISTENTSERVICE);
        PendingIntent sender = PendingIntent.getBroadcast(PersistentService.this, 0, intent, 0);

        long firstTime = SystemClock.elapsedRealtime();
        firstTime += REBOOT_DELAY_TIMER; // 10초 후에 알람이벤트 발생

        AlarmManager am = (AlarmManager) getSystemService(ALARM_SERVICE);
        am.setRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP, firstTime, REBOOT_DELAY_TIMER, sender);
    }

    /**
     * @Method Name    :  unregisterRestartAlarm
     * @Method 기능    :  기존 등록되어있는 알람을 해제한다.
     * @변경이력        :
     */
    private void unregisterRestartAlarm() {

        Log.d("PersistentService", "unregisterRestartAlarm()");
        Intent intent = new Intent(PersistentService.this, RestartService.class);
        intent.setAction(RestartService.ACTION_RESTART_PERSISTENTSERVICE);
        PendingIntent sender = PendingIntent.getBroadcast(PersistentService.this, 0, intent, 0);

        AlarmManager am = (AlarmManager) getSystemService(ALARM_SERVICE);
        am.cancel(sender);
    }

    /**
     * @Method Name    :  notifiyConnection
     * @Method 기능    :  소켓 연결이 되었을 때 알림 메시지를 사용자에게 보여준다.
     * @변경이력        :
     */
    private void notifiyConnection(String contentTitle, String contentText){

        // 노티피케이션
        NotificationManager mManager = (NotificationManager)getSystemService((Context.NOTIFICATION_SERVICE));
        // 노티피케이션 객체 생성
        Notification n = new Notification();
        // 아이콘 서정
        n.icon = R.drawable.ic_cast_dark;
        // 발생 즉시 잠시 보여질 내용
        n.tickerText = "New maessge";
        // 발생 시간
        n.when = System.currentTimeMillis();
        // 발생 수량 설정
        n.number = 0;
        // 알람 해제 방법을 설정
        n.flags = Notification.FLAG_AUTO_CANCEL;
        // 확장된 상태바에 나타낼 제목과 내용
        // 확장된 상태바에 놀렀을때 이동할 액티비티 설정
        Intent i = new Intent(getApplicationContext(), MainActivity.class);
        PendingIntent pi = PendingIntent.getActivities(this,0, new Intent[]{i}, 0);
        // 확장된 상태 표시줄 표시 설정
        n.setLatestEventInfo(getApplicationContext(), contentTitle, contentText, pi);
        //Notification 발생
        mManager.notify(0, n);

    }

    /**
     * @FileName        : PersistentService.java
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
                if (socket != null) {
                    //notifiyConnection("Security Bootloater", "중개서버와 연결이 되었습니다. 확인하시겠습니까?");
                    connection();
                }

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
                        socket.close();
                        socket = null;
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
                    Log.d("Protocol", "Booting Request");
                    Log.e("Protocol", "Booting Request");

                    notifiyConnection("Security Bootloater", "컴퓨터의 부팅이 감지 되었습니다.");
                    
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

            int buffSize = 8 + data.length() + 1;

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

            int buffSize = 8 + Integer.SIZE / 8 + data.length() + 1;

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
                socket.close();
                socket = null;
            } catch (IOException e2) {
                Log.e("ERROR::connection()", e2.getMessage());
            }

            super.onPostExecute(result);
        }
    }
}
