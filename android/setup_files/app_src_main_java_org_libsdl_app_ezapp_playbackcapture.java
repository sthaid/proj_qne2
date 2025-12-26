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

// ... inside your Activity or Service ...

public class ezapp_playbackcapture {

    private static final String TAG = "SDL";

    private AudioRecord audioRecord = null;
    private Thread recordingThread = null;
    private boolean isRecording = false;
    private MediaProjection mediaProjection; // Assume this is initialized from a MediaProjectionManager request

    // Configuration for the audio format
    private static final int SAMPLE_RATE = 44100;
    private static final int CHANNEL_CONFIG = AudioFormat.CHANNEL_IN_MONO; // Mono is sufficient for mixed output
    private static final int AUDIO_FORMAT = AudioFormat.ENCODING_PCM_16BIT;
    private int bufferSizeInBytes;

    private Context cx;

    private static final int PERMISSION_CODE = 1; //xxx what is this

    public ezapp_playbackcapture(Context cxarg) {
        Log.v(TAG, "EZAPP playbackcapture init");
        cx = cxarg;

        MediaProjectionManager mProjectionManager = 
            (MediaProjectionManager) cx.getSystemService(Context.MEDIA_PROJECTION_SERVICE);

        //startActivityForResult(mProjectionManager.createScreenCaptureIntent(), PERMISSION_CODE);
        // xxx User granted permission, get the MediaProjection instance
 
        Intent intent = mProjectionManager.createScreenCaptureIntent();
        mediaProjection = mProjectionManager.getMediaProjection(-1, intent); //xxx ?? resultCode, data);
// add prints
// update xml
// set the permission manually, and/or from within SDL main.c
// call the constructor at a later time, triggered by code in main.c
// call startPlaybackCapture,  and view pritns
// add prints to the thread
    }

    public void startPlaybackCapture() {
        if (mediaProjection == null) {
            // Handle case where MediaProjection is not available (e.g., user denied permission)
            return;
        }

        bufferSizeInBytes = AudioRecord.getMinBufferSize(SAMPLE_RATE, CHANNEL_CONFIG, AUDIO_FORMAT);

        // 1. Build the AudioPlaybackCaptureConfiguration
        AudioPlaybackCaptureConfiguration captureConfig =
            new AudioPlaybackCaptureConfiguration.Builder(mediaProjection)
                .addMatchingUsage(AudioAttributes.USAGE_MEDIA) // Capture media playback audio
                .addMatchingUsage(AudioAttributes.USAGE_GAME)  // Capture game audio
                .build();

        // 2. Configure the AudioFormat
        AudioFormat format = new AudioFormat.Builder()
            .setSampleRate(SAMPLE_RATE)
            .setChannelMask(CHANNEL_CONFIG)
            .setEncoding(AUDIO_FORMAT)
            .build();

        // 3. Initialize AudioRecord with the configuration
        audioRecord = new AudioRecord.Builder()
            //xxx .setAudioSource(MediaRecorder.AudioSource.DEFAULT) // Default source is fine for playback capture
            .setAudioFormat(format)
            .setBufferSizeInBytes(bufferSizeInBytes)
            //xxx .setAudioPlaybackCaptureConfiguration(captureConfig) // **Key step**
            .build();

        // Start recording
        audioRecord.startRecording();
        isRecording = true;

        // Start a thread to read the audio data
        recordingThread = new Thread(new Runnable() {
            @Override
            public void run() {
                writeAudioDataToFile();
            }
        }, "AudioRecorderThread");
        recordingThread.start();
    }

    private void writeAudioDataToFile() {
        // This is a simple example for saving raw PCM data.
        // Real-world apps might use an encoder or a library to create a proper audio file (e.g., WAV).
        File file = new File(cx.getExternalFilesDir(null), "captured_audio.pcm");
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(file);
            byte[] buffer = new byte[bufferSizeInBytes];
            int bytesRead;

            while (isRecording) {
                bytesRead = audioRecord.read(buffer, 0, bufferSizeInBytes);
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

    private void stopPlaybackCapture() {
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
