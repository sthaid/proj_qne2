package org.libsdl.app;

import android.content.Context;
import android.util.Log;
import android.os.Binder;
import android.os.IBinder;

import android.speech.tts.TextToSpeech;
import java.util.Locale;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationResult;
import android.location.Location;
import android.os.Looper;

public class ezapp_utils {
    private static final   String TAG = "SDL";
    private static         TextToSpeech mTts;
    private static boolean isTtsInitialized = false;

    private FusedLocationProviderClient fusedLocationClient;
    private LocationCallback            locationCallback;
    private double                      latitude = 999999999;
    private double                      longitude = 999999999;
    private double                      altitude = 999999999;

    public ezapp_utils(Context cx) {
        Log.v(TAG, "EAAPP_utils init");

        // Initialize TextToSpeech support
        mTts = new TextToSpeech(cx, new TextToSpeech.OnInitListener() {
            @Override
            public void onInit(int status) { 
                if (status != TextToSpeech.ERROR) {
                    int result = mTts.setLanguage(Locale.US);
                    if (result == TextToSpeech.LANG_MISSING_DATA || result == TextToSpeech.LANG_NOT_SUPPORTED) {
                        Log.v(TAG, "EZAPP ERROR TTS Lang not supported");
                    } else {
                        isTtsInitialized = true;
                    }
                }
            } 
        });

        // Initialize Location support
        fusedLocationClient = LocationServices.getFusedLocationProviderClient(cx);

        locationCallback = new LocationCallback() {
            @Override
            public void onLocationResult(LocationResult locationResult) {
                if (locationResult == null) {
                    return;
                }
                for (Location location : locationResult.getLocations()) {
                    latitude = location.getLatitude();
                    longitude = location.getLongitude();
                    altitude = location.getAltitude();
                    Log.v(TAG, "EZAPP lat/long/alt = " + latitude + " " + longitude + " " + altitude);
                }
            }
        };

        // start location updates using 60 second interval
        LocationRequest locationRequest = LocationRequest.create();
        locationRequest.setInterval(60*1000);
        locationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);
        fusedLocationClient.requestLocationUpdates(locationRequest, locationCallback, Looper.getMainLooper());
    }

    public void destroy() {
        Log.v(TAG, "EAAPP_utils destroy");
        if (mTts != null) {
            mTts.stop();
            mTts.shutdown();
            mTts = null;
        }
    }

    public int text_to_speech(String message) {
        int status;
        if (isTtsInitialized && mTts != null) {
            if (message.length() > 0) {
                Log.v(TAG, "EZAPP tts speaking: " + message);
                status = mTts.speak(message, TextToSpeech.QUEUE_FLUSH, null, "utteranceId1"); // xxx what are the args
            } else {
                Log.v(TAG, "EZAPP tts stopping");
                status = mTts.stop();
            }
            return status;
        } else {
            return -1;
        }
    }

    public double get_latitude() {
        return latitude;
    }

    public double get_longitude() {
        return longitude;
    }

    public double get_altitude() {
        return altitude;
    }
}
