package com.example.kimhajin.securitybootloader.Encryption;

/**
 * Created by 주현 on 2016-03-04.
 */
public interface PacketSecurity {

    public String incode(String buf);
    public String decode(String buf);
}
