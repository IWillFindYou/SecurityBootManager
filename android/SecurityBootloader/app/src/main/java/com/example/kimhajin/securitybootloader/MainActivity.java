package com.example.kimhajin.securitybootloader;

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.example.kimhajin.securitybootloader.Network.PersistentService;
import com.example.kimhajin.securitybootloader.Network.RestartService;
import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.MarkerOptions;

import java.util.UUID;

/** 
 * @FileName		: MainActivity.java 
 * @Project		: SecurityBootloder 
 * @Date			: 2015. 8. 24. 
 * @작성자			: 주현 
 * @프로그램 설명		: main 클레스
 * @프로그램 기능		: 
 * @변경이력		: 
 */

public class MainActivity extends FragmentActivity implements View.OnClickListener {

    private GoogleMap googleMap;
    // Background Service
    private BroadcastReceiver receiver;
    private Intent intentMyService;
    // button nameing
    private Button alert;
    private Button alert2;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        //app 실행시 Splash 먼저 실행되게 함. /
        Log.v("msss", "deviceID" + getDevicesUUID(this));

        //Background service //
        Log.d("MpMainActivity", "service start!!");
        //immortal service 등록
        intentMyService = new Intent(this, PersistentService.class);
        //========================================== 리시버 등록 ======================================== //

        // 실제로는 GPS 구현 없음 클래스 코드 간략화를 위해 GPS 관련 코드 삭제
        receiver = new RestartService();
        try {
            IntentFilter mainFilter = new IntentFilter("com.hamon.GPSservice.sss");
            // 리시버 저장
            registerReceiver(receiver, mainFilter);
            // 서비스 시작
            startService(intentMyService);
        } catch (Exception e) {
            Log.d("MyMainActivity", e.getMessage() + "");
            e.printStackTrace();
        }

        //========================================== 스플레시 시작 ======================================== //
        startActivity(new Intent(this, Splash.class));


        //------------------------ button 팝업창 설정 --------------------------------//
        //팝업버튼 boot 설정
        alert = (Button) findViewById(R.id.alert); //팝업버튼 아이디
        alert.setOnClickListener(this);
        //팝업버튼 shutdown 설정
        alert2 = (Button) findViewById(R.id.alert2); //팝업버튼 아이디
        alert2.setOnClickListener(this);
        //-------------------------------------------------------------------------//

    }
    public void OnDestroy(){
        // OnDestroy() : Actiivity가 종료되기 전 호출된다.
        //onCreate()와 짝을 이뤄 사용했던 리소스는 이 곳에서 싹~ 치워준다.
        // 리시버 삭제를 하지 않으면 에러
        Log.d("MyMainActivity", "Service Destroy");
        unregisterReceiver(receiver);
        super.onDestroy();
    }
    // ====================================== button 기능 구현 ============================================== //

    public void onClick(View view) {
        if(view==alert) { //view가 alert이면 팝업실행 즉 버튼 누르면 팝업창 뜨게 만드는 조건문
            new AlertDialog.Builder(this).setIcon(android.R.drawable.ic_dialog_alert)
                    .setTitle("Booting start") //팝업창 타이틀바
                    .setMessage("컴퓨터를 부팅하시겠습니까??")
                    .setPositiveButton("No", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                           // no 버튼 누르면 팝업창 닫힘 !!
                            Toast.makeText(getApplicationContext(), "취소되었습니다.", Toast.LENGTH_SHORT).show();
                        }
                    })
                    .setNeutralButton("Yes", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            //부팅 시작 Boot문장을 -> 중계서버로 보내주는 부분 구현 해야함 !!
                            Intent intent = new Intent(getApplicationContext(), PersistentService.class);
                            intent.putExtra("startBtn", true);
                            startService(intent);
                            MainActivity.this.moveTaskToBack(true);
                        }
                    })
                    .show(); //팝업창 보여줌
        }
        if(view==alert2) { //view가 alert이면 팝업실행 즉 버튼 누르면 팝업창 뜨게 만드는 조건문
            new AlertDialog.Builder(this).setIcon(android.R.drawable.ic_dialog_alert)
                    .setTitle("Shutdown") //팝업창 타이틀바
                    .setMessage("컴퓨터를 종료하시겠습니까?")
                    .setPositiveButton("No", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            //전화면으로 이동
                            Toast.makeText(getApplicationContext(), "취소되었습니다.", Toast.LENGTH_SHORT).show();
                        }
                    })
                    .setNeutralButton("Yes", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            //shutdown 문장을 -> 중계서버로 보내주는 부분 구현 해야함 !!
                            Intent intent = new Intent(getApplicationContext(), PersistentService.class);
                            intent.putExtra("cancelBtn", true);
                            startService(intent);
                            MainActivity.this.moveTaskToBack(true);
                        }
                    })
                    .show(); //팝업창 보여줌
        }
    }
    // ===================================================================================================== //

    //============================================= MAP 관련 부분 =============================================//
    @Override
    protected void onResume()
    {
        // Activity가 전면에 나타날 때 대부분의 상황에서 호출된다. 처음 실행했을 때, onCreate() 이후에도 호출된다.
        // onCreate()에 googleMap을 필요함수를 넣으면 어플이 죽는것을 확인 !! 그래서 onResume()함수에 넣어 동작 !!
        super.onResume();
        init();
    }
    void init()
    {
        if(googleMap == null)
        {
            googleMap = ((SupportMapFragment)getSupportFragmentManager().findFragmentById(R.id.map)).getMap();

            if(googleMap != null)
            {
                addMarker();
                setCamera();
            }
        }
    }
    // map Camera Zoom and move 함수
    void setCamera()
    {
        //LatLng(latitude 위도 , longitude 경도)
        //맵위치 이동 및 카메라 줌 (LatLng 위도 경도) 5.0f = Camera Zoom 단계
        googleMap.moveCamera((CameraUpdateFactory.newLatLngZoom(new LatLng(37, 128), 5.0f)));
    }
    // Marker add 함수
    void addMarker()
    {
        // 마커 지정 ( 위도 경도 ) 마커랑 맵위치 랑 같은 위도 경도를 가져아함 !!
        googleMap.addMarker(new MarkerOptions().position(new LatLng(37,128))).setTitle("sample Marker");
    }
    // =================================================================================================== //

    // 디바이스 고유 ID 가져온다 !!
    private String getDevicesUUID(Context mContext){
        final TelephonyManager tm = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
        final String tmDevice, tmSerial, androidId;
        tmDevice = "" + tm.getDeviceId();
        tmSerial = "" + tm.getSimSerialNumber();
        androidId = ""
                + android.provider.Settings.Secure.getString(
                getContentResolver(),
                android.provider.Settings.Secure.ANDROID_ID);

        UUID deviceUuid = new UUID(androidId.hashCode(),

                ((long) tmDevice.hashCode() << 32) | tmSerial.hashCode());

        String deviceId = deviceUuid.toString();
        //Log.v("msss", "deviceID" + deviceId);
        return deviceId;

    }
}