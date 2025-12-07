#include <utils.h>
#include <logging.h>

#define INVALID_NUMBER 999999999  // xxx get this from sdlx.h; or fix picoc so NAN/isnan works

#ifdef ANDROID

#include <SDL3/SDL.h>
#include <jni.h>
#include <unistd.h>

// The following comment is copied from here:
//   https://wiki.libsdl.org/SDL3/SDL_GetAndroidActivity
//
// Warning (and discussion of implementation details of SDL for Android):
// Local references are automatically deleted if a native function called
// from Java side returns. For SDL this native function is main() itself.
// Therefore references need to be manually deleted because otherwise the
// references will first be cleaned if main() returns (application exit).

// -----------------  ANDROID - GET LOCATION -------------------------

// Notes on altitude, from Google AI Overview:
//  "GPS altitude is a height above the WGS84 reference ellipsoid,
//   which is an approximation of the Earth's surface. This value is
//   not the same as height above mean sea level and may require a
//   correction, according to Stack Overflow"
//   https://stackoverflow.com/questions/11168306/is-androids-gps-altitude-incorrect-due-to-not-including-geoid-height

// args:
// - latitude:  degress, north latitude is positive
// - longitude: degress, east longitude is positive
// - altitude:  meters, accuracy is 10-20 meters
void util_get_location(double *latitude, double *longitude, double *altitude)
{
    jmethodID method_id;
    int tries = 0;

try_again:
    // preset return values to invalid
    if (latitude)  *latitude = INVALID_NUMBER;
    if (longitude) *longitude = INVALID_NUMBER;
    if (altitude)  *altitude = INVALID_NUMBER;

    // retrieve the JNI environment.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();

    // retrieve the Java instance of the SDLActivity
    jobject activity = (jobject)SDL_GetAndroidActivity();

    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    jclass clazz(env->GetObjectClass(activity));

    // get the method_id and call the methods to get location information
    if (latitude) {
        method_id = env->GetMethodID(clazz, "get_latitude", "()D");
        if (method_id != 0) {
            *latitude = env->CallDoubleMethod(activity, method_id);
        }
    }
    if (longitude) {
        method_id = env->GetMethodID(clazz, "get_longitude", "()D");
        if (method_id != 0) {
            *longitude = env->CallDoubleMethod(activity, method_id);
        }
    }
    if (altitude) {
        method_id = env->GetMethodID(clazz, "get_altitude", "()D");
        if (method_id != 0) {
            *altitude = env->CallDoubleMethod(activity, method_id);
        }
    }

    // clean up the localreferences.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);

    // retry if latitude, longitude or altitude are invalid
    if ((latitude && (*latitude == 0 || *latitude == INVALID_NUMBER)) ||
        (longitude && (*longitude == 0 || *longitude == INVALID_NUMBER)) ||
        (altitude && (*altitude == INVALID_NUMBER))) 
    {
        if (tries++ == 10) {
            if (latitude)  *latitude = INVALID_NUMBER;
            if (longitude) *longitude = INVALID_NUMBER;
            if (altitude)  *altitude = INVALID_NUMBER;
        } else {
            INFO("retrying get lat,long,alt\n");
            sleep(1);
            goto try_again;
        }
    }
}

// -----------------  ANDROID - TEXT TO SPEECH  ----------------------

void util_text_to_speech(char *text)
{
    jmethodID method_id;
    int rc;

    // retrieve the JNI environment.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();

    // retrieve the Java instance of the SDLActivity
    jobject activity = (jobject)SDL_GetAndroidActivity();

    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    jclass clazz(env->GetObjectClass(activity));

    // goolge search "what are the args to GetMethodID" ...
    //
    // Example of a method signature:
    // - (I)V: A method that takes an int as a parameter and returns void.
    // - (Ljava/lang/String;)I: A method that takes a String object as a parameter and returns an int.
    // - (Ljava/lang/String;I)V: A method that takes a String and an int as parameters and returns void.

    // get the method_id, print message if failed
    method_id = env->GetMethodID(clazz, "text_to_speech", "(Ljava/lang/String;)I");
    if (method_id == 0) {
        ERROR("failed to get method_id for text_to_speech\n");
    }

    // if got the method_id then ...
    if (method_id != 0) {
        // Convert C string to Java String
        //
        // Note - When using JNI's NewStringUTF function, you are creating a new java.lang.String
        //        object within the Java Virtual Machine (JVM). This jstring is a local reference,
        //        and its memory management is handled by the JVM's garbage collector.
        jstring java_string = env->NewStringUTF(text);

        // call text_to_speech method
        rc = env->CallIntMethod(activity, method_id, java_string);
        if (rc != 0) {
            ERROR("text_to_speech failed, rc=%d\n", rc);
        }
    }

    // clean up the localreferences.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);
}

void util_text_to_speech_stop(void)
{
    char stop[] = "";
    util_text_to_speech(stop);
}

// -----------------  ANDROID - START/STOP FGSVC  --------------------

// xxx comment 

// xxx maybe - if its param is enabled, then dont allow it to be stopped
// by an application call

void util_start_foreground(void)
{
    jmethodID method_id;
    int rc;

    // retrieve the JNI environment.,
    // retrieve the Java instance of the SDLActivity,
    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz(env->GetObjectClass(activity));

    // get the method_id, print message if failed
    method_id = env->GetMethodID(clazz, "start_foreground", "()V");
    if (method_id == 0) {
        ERROR("failed to get method_id for start_foreground\n");
    }

    // if got the method_id then call the start_foreground method
    if (method_id != 0) {
        env->CallVoidMethod(activity, method_id);
    }

    // clean up the localreferences.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);
}

void util_stop_foreground(void)
{
    jmethodID method_id;
    int rc;

    // retrieve the JNI environment;
    // retrieve the Java instance of the SDLActivity;
    // find the Java class of the activity. It should be SDLActivity or a subclass of it
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz(env->GetObjectClass(activity));

    // get the method_id, print message if failed
    method_id = env->GetMethodID(clazz, "stop_foreground", "()V");
    if (method_id == 0) {
        ERROR("failed to get method_id for stop_foreground\n");
    }

    // if got the method_id then call the stop_foreground method
    if (method_id != 0) {
        env->CallVoidMethod(activity, method_id);
    }

    // clean up the localreferences.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);
}

bool util_is_foreground_enabled(void)
{
    jmethodID method_id;
    int rc;
    bool is_fg_enabled = false;

    // retrieve the JNI environment;
    // retrieve the Java instance of the SDLActivity;
    // find the Java class of the activity. It should be SDLActivity or a subclass of it
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz(env->GetObjectClass(activity));

    // get the method_id, print message if failed
    method_id = env->GetMethodID(clazz, "is_foreground_enabled", "()Z");
    if (method_id == 0) {
        ERROR("failed to get method_id for stop_foreground\n");
    }

    // if got the method_id then call the is_foreground_enabled method
    if (method_id != 0) {
        is_fg_enabled = env->CallBooleanMethod(activity, method_id);
    }

    // clean up the localreferences.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);

    // return is_fg_enabled flag
    return is_fg_enabled;
}

#else

// -----------------  NOT ANDROID - TEST CODE  ---------------------------

#define BOLTON_MASS_LATITUDE     42.4334
#define BOLTON_MASS_LONGITUDE   -71.6078
#define BOLTON_MASS_ELEVATION    100    // range is 63 to 201 meters

void util_get_location(double *latitude, double *longitude, double *altitude)
{
    if (latitude) {
        *latitude = BOLTON_MASS_LATITUDE;
    }
    if (longitude) {
        *longitude = BOLTON_MASS_LONGITUDE;
    }
    if (altitude) {
        *altitude = BOLTON_MASS_ELEVATION;
    }
}

void util_text_to_speech(char *text) { }
void util_text_to_speech_stop(void) { }
void util_start_foreground(void) { }
void util_stop_foreground(void) { }
bool util_is_foreground_enabled(void) { return false; }

#endif
