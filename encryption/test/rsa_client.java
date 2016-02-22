import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class rsa_client {

	public static void main(String[] argv) {

		try {

			String serverIP = "127.0.0.1"; // 127.0.0.1 & localhost 본인
			System.out.println("서버에 연결중입니다. 서버 IP : " + serverIP);

			// 소켓을 생성하여 연결을 요청한다.
			Socket socket = new Socket(serverIP, 15243);
			System.out.println("서버에 연결되었습니다.");

			DataOutputStream dos = null;
			DataInputStream dis = null;

			dos = new DataOutputStream(new BufferedOutputStream(socket.getOutputStream()));
			dis = new DataInputStream(new BufferedInputStream(socket.getInputStream()));
			
			// SEND
			String message = "abcdefghijklmnopqrstuvwuyz0123456789\0";
			String rsaMessage = rsa_coding(143, 7, message);
			System.out.println("Send Msg : " + message);
			System.out.println("Incoding Send Msg : " + rsaMessage);
			
			byte[] byteMessage = new byte[1024];
			byteMessage = rsaMessage.getBytes();
			dos.write(byteMessage);
			dos.flush();

			// RECV
			byteMessage = new byte[1024];
			// 메지시를 읽어 바이트 배열에 저장
			dis.read(byteMessage, 0, byteMessage.length);
			
			rsaMessage = byteToString(byteMessage, ByteOrder.LITTLE_ENDIAN);
			message = rsa_coding(143, 103, rsaMessage);
			System.out.println("Recv MSG : " + rsaMessage);
			System.out.println("Decoding Recv MSG : " + message);

			dos.close();
			dis.close();
			socket.close();

		} catch (UnknownHostException e) {
			System.out.println("Unkonw exception " + e.getMessage());

		} catch (IOException e) {
			System.out.println("IOException caught " + e.getMessage());
			e.printStackTrace();
		}
	}

	public static String byteToString(byte[] bytes, ByteOrder order) {

		ByteBuffer buff = ByteBuffer.allocate(bytes.length);
		buff.order(order); // Set big/little endian
		buff.put(bytes); // Insert byte value to ByteBuffer
		buff.position(0); // position(0) 실행 되면 position은 0에 위치 하게 됨.

		byte[] temp = new byte[bytes.length];
		buff.get(temp);

		String result = new String(temp);

		return result;
	}

	public static String rsa_coding(int key1, int key2, String buff) {

		int i, j, k;
		int[] temp = new int[1024];
		StringBuffer result = new StringBuffer();

		for (i = 0; buff.charAt(i) != 0; i++) {
			temp[i] = buff.charAt(i);
			if (temp[i] < 0)
				temp[i] = 256 + temp[i];
		}

		i = 0;
		while (buff.charAt(i) != 0) {
			
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
