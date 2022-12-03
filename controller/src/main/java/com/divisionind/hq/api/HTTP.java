package com.divisionind.hq.api;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.net.HttpURLConnection;
import java.net.URL;

public class HTTP {

    public static HTTP request(String url, String method, String body) throws IOException {
        HttpURLConnection conn = (HttpURLConnection) new URL(url).openConnection();

        // configure headers
        conn.setRequestMethod(method);
        conn.setRequestProperty("Content-Type", "application/json");

        // write body
        if (body != null) {
            conn.setDoOutput(true);
            PrintStream out = new PrintStream(conn.getOutputStream());
            out.print(body);
            out.flush();
            out.close();
        }

        // read response
        int code = conn.getResponseCode();
        BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream()));
        String line;
        StringBuilder sb = new StringBuilder();
        while ((line = reader.readLine()) != null) {
            sb.append(line);
        }
        reader.close();
        conn.disconnect();

        return new HTTP(code, sb.toString());
    }

    private final int code;
    private final String resp;

    public HTTP(int code, String resp) {
        this.code = code;
        this.resp = resp;
    }

    public int getCode() {
        return code;
    }

    public String getResponse() {
        return resp;
    }
}
