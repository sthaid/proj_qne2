package org.libsdl.app; // Use your project's package

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.widget.Toast;
import android.util.Log;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.graphics.Color;

import android.content.pm.ServiceInfo;

import org.sthaid.ezApp.R;


//import androidx.ServiceCompat;

//import androidx.app.ServiceCompat;
//import android.app.Notification;
//import android.app.NotificationCompat;
//import android.app.Service;

// https://developer.android.com/develop/background-work/services/fgs/launch

public class MyService extends Service {

    private static final String TAG = "SDL";

//  @Override
//  protected void onCreate() {
//      Log.v(TAG, "XXX in MyService onCreate");
//      super.onCreate();
//  }
    static boolean destroy;

    static class MyExtendedThread extends Thread {

        private static final String TAG = "SDL";

        @Override
        public void run() {
            // Example: Simulate a long task
            try {
                while (true) {
                    Log.v(TAG, "XXX in thread before sleep 2 secs");
                    Thread.sleep(2000);
                    Log.v(TAG, "XXX in thread after sleep");

                    if (destroy) {
                        break;
                    }
                }
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        // To start the thread (simply call start() on an instance of this class)
        // MyExtendedThread thread = new MyExtendedThread();
        // thread.start();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String CHANNEL_ID = "my_channel_id";
        String CHANNEL_NAME = "My Channel";
        String CHANNEL_DESCRIPTION = "Description for My Channel";

        Log.v(TAG, "XXX in MyService onStartCommand");
        Toast.makeText(this, "Service started by SDL activity", Toast.LENGTH_LONG).show();


        NotificationManager notificationManager = (NotificationManager) this.getSystemService(this.NOTIFICATION_SERVICE);

        // Create a Notification Channel for Android O and above
// xxx put the build stuff back in

        NotificationChannel channel = new NotificationChannel(CHANNEL_ID, CHANNEL_NAME, NotificationManager.IMPORTANCE_DEFAULT);

        channel.setDescription(CHANNEL_DESCRIPTION);
        //channel.enableLights(true);
        //channel.setLightColor(Color.RED);
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
        int type_xxx = ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION;
        this.startForeground(100, notification, type_xxx);

        // Add your background tasks here. For example, a network request.
        // For a long-running task, you should use a separate thread.
        // It's the developer's responsibility to stop the service when finished.
        // stopSelf();

        Log.v(TAG, "XXX in MyService onStartCommand create thread");
        MyExtendedThread thrd = new MyExtendedThread(); 
        thrd.start();

        Log.v(TAG, "XXX in MyService onStartCommand  DONE");
        // xxx do we really want to restart here
        return START_STICKY; // Service will be restarted if killed by the system
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null; // Not a bound service
    }

    @Override
    public void onDestroy() {
        Log.v(TAG, "XXX in MyService OnDestroy, setting destroy");
        destroy = true;
        // Toast.makeText(this, "Service destroyed", Toast.LENGTH_LONG).show();
        // super.onDestroy();
    }
}


//import android.os.Handler;
//import android.os.Looper;

