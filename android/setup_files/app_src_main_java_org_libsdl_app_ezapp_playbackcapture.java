package org.libsdl.app;

import android.content.Context;
import android.util.Log;
import android.content.Intent;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioPlaybackCaptureConfiguration;
import android.media.AudioRecord;
import android.media.projection.MediaProjection;
import android.os.ParcelFileDescriptor;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

import android.media.projection.MediaProjectionManager;
import android.os.SystemClock;

import android.media.AudioPlaybackCaptureConfiguration;

// ... inside your Activity or Service ...

public class ezapp_playbackcapture {

    private static final String TAG = "SDL";

    private AudioRecord audioRecord = null;
    private Thread recordingThread = null;
    private boolean isRecording = false;

    // Configuration for the audio format
    private static final int SAMPLE_RATE = 44100;
    private static final int CHANNEL_CONFIG = AudioFormat.CHANNEL_IN_MONO; // Mono is sufficient for mixed output
    private static final int AUDIO_FORMAT = AudioFormat.ENCODING_PCM_16BIT;
    private int bufferSizeInBytes;

    private Context cx;

    private static final int PERMISSION_CODE = 1234; //xxx what is this
    private MediaProjectionManager mProjectionManager;
    private MediaProjection mediaProjection; // Assume this is initialized from a MediaProjectionManager request

    private Intent mdata;

    public void on_result(int requestCode, int resultCode, Intent data) {
        Log.v(TAG, "XXXXXXXXXX in onActivityResult requestCode " + requestCode);
        Log.v(TAG, "XXXXXXXXXX in onActivityResult resultCode " + resultCode);

        if (requestCode != PERMISSION_CODE) {
            // Handle error or unknown request code
            return;
        }
        if (resultCode != -1) {  // xxx RESULT_OK
            // User denied permission
            return;
        }

        mdata = data;

        // User granted permission, get the MediaProjection instance
        //mediaProjection = mProjectionManager.getMediaProjection(resultCode, data);
        //Log.v(TAG, "XXXXXXXXXX mediaProjection = " + mediaProjection);

        // Proceed to start a foreground service and create a VirtualDisplay
        // (especially for Android 14+ or long-running tasks)
        //startMediaProjectionSession(mediaProjection);

        //startPlaybackCapture();
    }

//  public ezapp_playbackcapture(SDLActivity mSingleton, Context cxarg) {
//      Log.v(TAG, "XXXXXXXXXX EZAPP playbackcapture init");
//      cx = cxarg;

//      mProjectionManager = 
//          (MediaProjectionManager) cx.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
//      Log.v(TAG, "XXXXXXXXXX mProjectionManger = " + mProjectionManager);

//      mSingleton.startActivityForResult(mProjectionManager.createScreenCaptureIntent(), PERMISSION_CODE);
//      Log.v(TAG, "XXXXXXXXXX after  mSingleton.startActivityForResult");

        // xxx User granted permission, get the MediaProjection instance
 
        //Intent intent = mProjectionManager.createScreenCaptureIntent();
        //mediaProjection = mProjectionManager.getMediaProjection(-1, intent); //xxx ?? resultCode, data);
// add prints
// update xml
// set the permission manually, and/or from within SDL main.c
// call the constructor at a later time, triggered by code in main.c
// call startPlaybackCapture,  and view pritns
// add prints to the thread

    public void startPlaybackCapture(SDLActivity mSingleton, Context cxarg) {
        Log.v(TAG, "XXXXXXXX startPlaybackCapture starting");
        cx = cxarg;

        mProjectionManager = 
            (MediaProjectionManager) cx.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        Log.v(TAG, "XXXXXXXXXX mProjectionManger = " + mProjectionManager);

        mSingleton.startActivityForResult(mProjectionManager.createScreenCaptureIntent(), PERMISSION_CODE);
        Log.v(TAG, "XXXXXXXXXX after  mSingleton.startActivityForResult");

        while (mdata == null) {
            SystemClock.sleep(2000);
        }

        mediaProjection = mProjectionManager.getMediaProjection(-1, mdata);
        if (mediaProjection == null) {
            Log.v(TAG, "XXXXXXXXXX getMediaProjection failed\n");
            return;
        }

//      if (mediaProjection == null) {
            // Handle case where MediaProjection is not available (e.g., user denied permission)
            //mediaProjection = mProjectionManager.getMediaProjection(resultCode, data);
//          Log.v(TAG, "XXXXXXXX startPlaybackCapture mediaProjection = " + mediaProjection);
//          if (mediaProjection == null) {
//              return;
//          }
//      }

        Log.v(TAG, "XXXXXXXX 1");
        bufferSizeInBytes = AudioRecord.getMinBufferSize(SAMPLE_RATE, CHANNEL_CONFIG, AUDIO_FORMAT);

        // 1. Build the AudioPlaybackCaptureConfiguration
        Log.v(TAG, "XXXXXXXX 2");
        AudioPlaybackCaptureConfiguration captureConfig =
            new AudioPlaybackCaptureConfiguration.Builder(mediaProjection)
                .addMatchingUsage(AudioAttributes.USAGE_MEDIA) // Capture media playback audio
                //.addMatchingUsage(AudioAttributes.USAGE_GAME)  // Capture game audio
                .build();

        // 2. Configure the AudioFormat
        Log.v(TAG, "XXXXXXXX 3");
        AudioFormat format = new AudioFormat.Builder()
            .setSampleRate(SAMPLE_RATE)
            .setChannelMask(CHANNEL_CONFIG)
            .setEncoding(AUDIO_FORMAT)
            .build();

        // 3. Initialize AudioRecord with the configuration
        Log.v(TAG, "XXXXXXXX 4");
        audioRecord = new AudioRecord.Builder()
            //.setAudioSource(MediaRecorder.AudioSource.DEFAULT) // Default source is fine for playback capture
            .setAudioFormat(format)
            .setBufferSizeInBytes(bufferSizeInBytes)
            .setAudioPlaybackCaptureConfig(captureConfig) // **Key step**
            .build();

        // Start recording
        Log.v(TAG, "XXXXXXXX 5");
        audioRecord.startRecording();
        isRecording = true;

        // Start a thread to read the audio data
        Log.v(TAG, "XXXXXXXX 6");
        recordingThread = new Thread(new Runnable() {
            @Override
            public void run() {
                writeAudioDataToFile();
            }
        }, "AudioRecorderThread");
        recordingThread.start();
        Log.v(TAG, "XXXXXXXX 7");
    }

    private void writeAudioDataToFile() {
        // This is a simple example for saving raw PCM data.
        // Real-world apps might use an encoder or a library to create a proper audio file (e.g., WAV).
        Log.v(TAG, "XXXX EXT FILE DIR " + cx.getExternalFilesDir(null));
        File file = new File(cx.getFilesDir(), "captured_audio.pcm");
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(file);
            byte[] buffer = new byte[bufferSizeInBytes];
            int bytesRead;

            while (isRecording) {
                bytesRead = audioRecord.read(buffer, 0, bufferSizeInBytes);
                Log.v(TAG, "XXXX bytesRead " + bytesRead);
                if (bytesRead > 0) {
                    fos.write(buffer, 0, bytesRead);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (fos != null) {
                    fos.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public void stopPlaybackCapture() {
        isRecording = false;
        if (recordingThread != null) {
            try {
                recordingThread.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null;
        }
        if (mediaProjection != null) {
            mediaProjection.stop();
            mediaProjection = null;
        }
    }
}
