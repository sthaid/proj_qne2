// References
// https://developer.android.com/develop/background-work/services/fgs/launch

package org.libsdl.app; // Use your project's package

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
//import android.widget.Toast;
import android.util.Log;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
//import android.graphics.Color;

import android.content.pm.ServiceInfo;

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

public class LocationService extends Service {

    private static final String TAG = "SDL";
    private FusedLocationProviderClient fusedLocationClient;
    private LocationCallback locationCallback;
    public double latitude;
    public double longitude;
    public double altitude = 999999999;
    private final IBinder mBinder = new InnerBinder();

    public class InnerBinder extends Binder {
        LocationService getService() {
            return LocationService.this;
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String CHANNEL_ID = "my_channel_id";
        String CHANNEL_NAME = "My Channel";
        String CHANNEL_DESCRIPTION = "Description for My Channel";

        Log.v(TAG, "XXX in LocationService onStartCommand");

        NotificationManager notificationManager = (NotificationManager) this.getSystemService(this.NOTIFICATION_SERVICE);

        // Create a Notification Channel for Android O and above
        NotificationChannel channel = 
            new NotificationChannel(CHANNEL_ID, CHANNEL_NAME, NotificationManager.IMPORTANCE_DEFAULT);

        channel.setDescription(CHANNEL_DESCRIPTION);
        channel.enableVibration(true);
        channel.setLockscreenVisibility(Notification.VISIBILITY_PUBLIC); 

        notificationManager.createNotificationChannel(channel);

        // Build the notification
        Notification.Builder builder;
        builder = new Notification.Builder(this, CHANNEL_ID);

        Notification notification = builder
                .setContentTitle("title_persist")
                .setContentText("message_persist")
                .setSmallIcon(R.drawable.ic_notifcation_icon)
                .setOngoing(true)
                .setPriority(Notification.PRIORITY_DEFAULT)
                .setAutoCancel(true)
                .setVisibility(Notification.VISIBILITY_PUBLIC)
                .build();

        // startForeground
        this.startForeground(100, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION);

        // -----------------------------------------

        Log.v(TAG, "XXX in LocationService calling getFusedLocationProviderClient");
        fusedLocationClient = LocationServices.getFusedLocationProviderClient(this);
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
                    Log.v(TAG, "XXX lat/long " + " " + latitude + " " + longitude + " " + altitude);
                    // Update UI or perform actions with latitude and longitude
                }
            }
        };

        startLocationUpdates();

        Log.v(TAG, "XXX in LocationService onStartCommand  DONE");
        return START_STICKY; // Service will be restarted if killed by the system
    }

    private void startLocationUpdates() {
        LocationRequest locationRequest = LocationRequest.create();
        locationRequest.setInterval(10000); // Update interval in milliseconds
        locationRequest.setFastestInterval(5000); // Fastest update interval
        locationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);

        //if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED) {
            fusedLocationClient.requestLocationUpdates(locationRequest, locationCallback, Looper.getMainLooper());
        //}
    }

    public double get_altitude() {
        Log.v(TAG, "XXX get_altitude return " + altitude);
        return altitude;
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.v(TAG, "XXX in LocationService IBinder-xxx");
        return mBinder;
    }

    @Override
    public void onDestroy() {
        Log.v(TAG, "XXX in LocationService OnDestroy");
    }
}
