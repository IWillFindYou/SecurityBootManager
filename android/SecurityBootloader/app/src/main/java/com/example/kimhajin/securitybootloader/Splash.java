package com.example.kimhajin.securitybootloader;
// Splash 구현 부분 //

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;

/** 
 * @FileName		: Splash.java 
 * @Project		: SecurityBootloder 
 * @Date			: 2015. 8. 24. 
 * @작성자			: 주현 
 * @프로그램 설명		: 어플리케이션이 최초 실행 되었을 때의 화면을 출력하는 클레스
 * @프로그램 기능		: 
 * @변경이력		: 
 */
public class Splash extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_splash);

        Handler hd = new Handler();
        hd.postDelayed(new Runnable() {
            @Override
            public void run() {
                finish(); //3s후 이미지 닫기
            }
        },2500);
    }
}
