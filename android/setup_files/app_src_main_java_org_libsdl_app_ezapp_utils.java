// xxx add make toast
package org.libsdl.app;

import android.util.Log;
import android.speech.tts.TextToSpeech;

//import android.os.LocaleList;
import java.util.Locale;

import android.content.Context;
//import android.content.SharedPreferences;
//import android.app.Application;

// xxx check these
import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationResult;

import android.location.Location;

import android.content.pm.PackageManager;  // xxx needed?

//import android.os.Handler; 
import android.os.Looper;

import android.os.Binder;
import android.os.IBinder;

import org.sthaid.ezApp.R;





public class ezapp_utils {
    private static final   String TAG = "SDL";  // xxx final?
    private static         TextToSpeech mTts;
    private static boolean isTtsInitialized = false;

    private FusedLocationProviderClient fusedLocationClient;
    private LocationCallback locationCallback;
    public double latitude = 999999999;
    public double longitude = 999999999;
    public double altitude = 999999999;

    public ezapp_utils(Context cx) {
        Log.v(TAG, "NEW XXX tester");
        Log.v(TAG, "NEW XXX tester");
        Log.v(TAG, "NEW XXX tester");
        Log.v(TAG, "NEW XXX tester");
        Log.v(TAG, "XXX tester");
        Log.v(TAG, "XXX tester");
        Log.v(TAG, "XXX tester");
        Log.v(TAG, "XXX tester");

        // Initialize TextToSpeech
        Log.v(TAG, "XXXXXXXXXXXXXX init tts");
        mTts = new TextToSpeech(cx, new TextToSpeech.OnInitListener() {
            @Override
            public void onInit(int status) { 
                Log.v(TAG, "XXXXXXXXXXXX TTS ONINIT CALLED");
                if (status != TextToSpeech.ERROR) {
                    // Set language (e.g., US English)
                    int result = mTts.setLanguage(Locale.US);
                    if (result == TextToSpeech.LANG_MISSING_DATA || result == TextToSpeech.LANG_NOT_SUPPORTED) {
                        //Toast.makeText(MainActivity.this, "Language not supported", Toast.LENGTH_SHORT).show();
                        Log.v(TAG, "XXXXXXXXXXXXXX TTS Lang not supported");
                    } else {
                        isTtsInitialized = true;
                        Log.v(TAG, "XXXXXXXXXXXXXX TTS INIT OKAY");
                    }
                } else {
                    ////Toast.makeText(MainActivity.this, "TTS Initialization failed", Toast.LENGTH_SHORT).show();
                    //Log.v(TAG, "XXXXXXXXXXXXXX TTS init failed");
                }
            } 
        });

        // xxxxxxxxxx lcoation

        Log.v(TAG, "XXX in LocationService calling getFusedLocationProviderClient");
        //fusedLocationClient = LocationServices.getFusedLocationProviderClient(this);
        fusedLocationClient = LocationServices.getFusedLocationProviderClient(cx);
        Log.v(TAG, "XXX in LocationService back from getFusedLocationProviderClient");

        locationCallback = new LocationCallback() {
            @Override
            public void onLocationResult(LocationResult locationResult) {
                Log.v(TAG, "XXX in LocationService onLocationResult");
                if (locationResult == null) {
                    Log.v(TAG, "XXX in LocationService onLocationResult IS NULL");
                    return;
                }
                for (Location location : locationResult.getLocations()) {
                    // Handle the location here
                    latitude = location.getLatitude();
                    longitude = location.getLongitude();
                    altitude = location.getAltitude();
                    Log.v(TAG, "NEW XXX lat/long " + " " + latitude + " " + longitude + " " + altitude);
                    // Update UI or perform actions with latitude and longitude
                }
            }
        };

        //startLocationUpdates();
        LocationRequest locationRequest = LocationRequest.create();
        locationRequest.setInterval(60*1000); // Update interval in milliseconds
        locationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);
        fusedLocationClient.requestLocationUpdates(locationRequest, locationCallback, Looper.getMainLooper());
    }

    public void destroy() {
        Log.v(TAG, "ezapp_utils: destroy");
        if (mTts != null) {
            mTts.stop();
            mTts.shutdown();
            mTts = null;
        }
    }

    public int text_to_speech(String message) {
        int status;
        Log.v(TAG, "NEW  XXXXXXXXXXXXXX SPEAKING " + message);
        if (isTtsInitialized && mTts != null) {
            if (message.length() > 0) {
                Log.v(TAG, "XXXXXXXXXXXXXX length " + message.length());
                status = mTts.speak(message, TextToSpeech.QUEUE_FLUSH, null, "utteranceId1"); // xxx what are the args
            } else {
                Log.v(TAG, "XXXXXXXXXXXXXX stopping");
                status = mTts.stop();
            }
            return status;
        } else {
            Log.v(TAG, "XXXXXXXXXXXXXX TTS NOT INIT");
            return -1;
        }
    }

    public double get_latitude() {
        Log.v(TAG, "EZZAPP XXX get_latitude return " + latitude);
        return latitude;
    }

    public double get_longitude() {
        Log.v(TAG, "EZZAPP XXX get_longitude return " + longitude);
        return longitude;
    }

    public double get_altitude() {
        Log.v(TAG, "EZZAPP XXX get_altitude return " + altitude);
        return altitude;
    }

}





/*
    private static final String TAG = "SDL";
    private final IBinder mBinder = new InnerBinder();

    public class InnerBinder extends Binder {
        LocationService getService() {
            return LocationService.this;
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.v(TAG, "XXX in LocationService onStartCommand");


        // -----------------------------------------


        Log.v(TAG, "XXX in LocationService onStartCommand  DONE");
        return START_STICKY; // Service will be restarted if killed by the system
    }

    private void startLocationUpdates() {
        LocationRequest locationRequest = LocationRequest.create();
        locationRequest.setInterval(60*1000); // Update interval in milliseconds
        //locationRequest.setMinUpdateIntervalMillis(LocationRequest.IMPLICIT_MIN_UPDATE_INTERVAL);  // min same as Interval
        //locationRequest.setFastestInterval(5000); // Fastest update interval
        locationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);

        //xxx why commented out
        //if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED) {
            fusedLocationClient.requestLocationUpdates(locationRequest, locationCallback, Looper.getMainLooper());
        //}
    }
*/
