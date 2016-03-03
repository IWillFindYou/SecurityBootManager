package com.example.kimhajin.securitybootloader.Encryption;

/**
 * Created by 주현 on 2016-03-04.
 */
public class RsaPacketSecurity implements PacketSecurity {

    public String incode(String buf){

        int n = 143, e = 7; // 추후 수정

        return rsa(n, e, buf);
    }

    public String decode(String buf){

        int n = 143, e = 103; // 추후 수정

        return rsa(n, e, buf);
    }

    private String rsa(int key1, int key2, String buf) {

        int i, j, k;
        int[] temp = new int[1024];
        StringBuffer result = new StringBuffer();

        for (i = 0; buf.charAt(i) != 0; i++) {
            temp[i] = buf.charAt(i);
            if (temp[i] < 0)
                temp[i] = 256 + temp[i];
        }

        i = 0;
        while (buf.charAt(i) != 0) {

            k = 1;
            for (j = 0; j < key2; j++) {
                k = k * temp[i];
                k = k % key1;
            }
            result.append((char) k);
            i++;
        }
        result.append('\0');

        return result.toString();
    }
}
