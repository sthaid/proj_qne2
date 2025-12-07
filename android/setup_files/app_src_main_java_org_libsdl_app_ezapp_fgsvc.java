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

//import com.google.android.gms.location.FusedLocationProviderClient;
//import com.google.android.gms.location.LocationServices;
//import com.google.android.gms.location.LocationRequest;
//import com.google.android.gms.location.LocationCallback;
//import com.google.android.gms.location.LocationResult;

//import android.location.Location;

import android.content.pm.PackageManager;  // xxx needed?

//import android.os.Handler; 
import android.os.Looper;

import android.os.Binder;
import android.os.IBinder;

import org.sthaid.ezApp.R;

public class ezapp_fgsvc extends Service {

    private static final String TAG = "SDL";
    //private FusedLocationProviderClient fusedLocationClient;
    //private LocationCallback locationCallback;
    //public double latitude = 999999999;
    //public double longitude = 999999999;
    //public double altitude = 999999999;
    private final IBinder mBinder = new InnerBinder();
    private static int running_count;

    public class InnerBinder extends Binder {
        ezapp_fgsvc getService() {
            return ezapp_fgsvc.this;
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.v(TAG, "XXX in ezapp_fgsvc onStartCommand");

        String CHANNEL_ID = "my_channel_id";
        String CHANNEL_NAME = "My Channel";
        String CHANNEL_DESCRIPTION = "Description for My Channel";

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

        running_count = running_count + 1;
        Log.v(TAG, "XXX in ezapp_fgsvc onStartCommand  DONE - running_count " + running_count);
        return START_STICKY; // Service will be restarted if killed by the system
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.v(TAG, "XXX in ezapp_fgsvc IBinder-xxx");
        return mBinder;
    }

    @Override
    public void onDestroy() { //xxx when is this called
        Log.v(TAG, "XXX in ezapp_fgsvc OnDestroy");
        Log.v(TAG, "XXX calling StopSelf");
        running_count = running_count - 1;
        Log.v(TAG, "XXX in ezapp_fgsvc Ondestroy  - running_count " + running_count);
        stopForeground(true);
        stopSelf();
    }
}
