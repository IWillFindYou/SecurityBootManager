package com.example.testclient;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/** 
 * @FileName		: ByteType.java 
 * @Project		: ByteType 
 * @Date			: 2015. 8. 24. 
 * @작성자			: yujoo 
 * @프로그램 설명		: 바이트 타입과 정수/문자열 간에 타입 변환
 * @프로그램 기능		: 
 *                          byte <=> Integer
 *                          byte <=> String
 * @변경이력		: 
 */
public class ByteType {
    /** 
     * @Method Name	: intTobyte 
     * @Method 기능	: int 타입을 byte 타입으로 변환
     * @변경이력		: 
     * @param integer
     * @param order
     * @return 
     */
    public static byte[] intToByte(int integer, ByteOrder order) {

        ByteBuffer buff = ByteBuffer.allocate(Integer.SIZE/8);
        buff.order(order);      // Set big/little endian
        buff.putInt(integer);   // Insert Integer value to ByteBuffer

        System.out.println("intTobyte : " + buff);

        return buff.array();
    }

    /** 
     * @Method Name	: byteToInt 
     * @Method 기능	: byte 타입을 int 타입으로 변환
     * @변경이력		: 
     * @param bytes
     * @param order
     * @return 
     */
    public static int byteToInt(byte[] bytes, ByteOrder order) {

        ByteBuffer buff = ByteBuffer.allocate(Integer.SIZE/8);
        buff.order(order);      // Set big/little endian
        buff.put(bytes);        // Insert byte value to ByteBuffer
        buff.position(0);       // position(0) 실행 되면 position은 0에 위치 하게 됨.

        System.out.println("byteToInt : " + buff);

        return buff.getInt(); // position위치(0)에서 부터 4바이트를 int로 변경하여 반환
    }

    /** 
     * @Method Name	: byteToString 
     * @Method 기능	: byte 타입을 String 타입으로 변환
     * @변경이력		: 
     * @param bytes
     * @param order
     * @return 
     */
    public static String byteToString(byte[] bytes, ByteOrder order) {

        ByteBuffer buff = ByteBuffer.allocate(bytes.length);
        buff.order(order);     // Set big/little endian
        buff.put(bytes);       // Insert byte value to ByteBuffer
        buff.position(0);      // position(0) 실행 되면 position은 0에 위치 하게 됨.

        byte[] temp = new byte[bytes.length];
        buff.get(temp);

        String result = new String(temp);

        System.out.println("byteToString : " + result);

        return result;
    }

    /** 
     * @Method Name	: stringToByte 
     * @Method 기능	: string 타입을 byte 타입으로 변환
     * @변경이력		: 
     * @param string
     * @param order
     * @return 
     */
    public static byte[] stringToByte(String string, ByteOrder order) {

        ByteBuffer buff = ByteBuffer.allocate(string.length());
        buff.order(order);              // Set big/little endian
        buff.put(string.getBytes());    // Insert byte value to ByteBuffer

        System.out.println("stringToByte : " + buff);

        return buff.array();
    }
}
