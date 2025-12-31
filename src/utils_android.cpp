// -----------------  ANDROID  ------------------------------------

#ifdef ANDROID

#define INVALID_NUMBER 999999999  // xxx get this from sdlx.h; or fix picoc so NAN/isnan works

#include <utils.h>
#include <logging.h>

#include <SDL3/SDL.h>
#include <jni.h>
#include <unistd.h>

// The following comment is copied from here:
//   https://wiki.libsdl.org/SDL3/SDL_GetAndroidActivity
// Warning (and discussion of implementation details of SDL for Android):
// Local references are automatically deleted if a native function called
// from Java side returns. For SDL this native function is main() itself.
// Therefore references need to be manually deleted because otherwise the
// references will first be cleaned if main() returns (application exit).

// Notes on altitude, from Google AI Overview:
//  "GPS altitude is a height above the WGS84 reference ellipsoid,
//   which is an approximation of the Earth's surface. This value is
//   not the same as height above mean sea level and may require a
//   correction, according to Stack Overflow"
//   https://stackoverflow.com/questions/11168306/is-androids-gps-altitude-incorrect-due-to-not-including-geoid-height

// JNI based mehtod signatures:
// References:
//  https://udaniweeraratne.wordpress.com/2016/07/10/how-to-generate-jni-based-method-signature/
//
// goolge search "what are the args to GetMethodID" ...
//   Example of a method signature:
//   - (I)V: A method that takes an int as a parameter and returns void.
//   - (Ljava/lang/String;)I: A method that takes a String object as a parameter and returns an int.
//   - (Ljava/lang/String;I)V: A method that takes a String and an int as parameters and returns void.

// prototype of common routine to call java method
static double call_java(const char *method_name, char *arg_str);

// location
void util_get_location(double *latitude, double *longitude, double *altitude) {
    if (latitude) {
        *latitude = call_java("get_latitude", NULL);
    }
    if (longitude) {
        *longitude = call_java("get_longitude", NULL);
    }
    if (altitude) {
        *altitude = call_java("get_altitude", NULL);
    }
}

// text to speech
void util_text_to_speech(char *text) {
    call_java("text_to_speech", text);
}
void util_text_to_speech_stop(void) {
    char text[1] = { '\0' };
    call_java("text_to_speech_stop", text);
}

// foreground service
void util_start_foreground(void) {
    call_java("start_foreground", NULL);
}
void util_stop_foreground(void) {
    call_java("stop_foreground", NULL);
}
bool util_is_foreground_enabled(void) {
    return call_java("is_foreground_enabled", NULL) == 1;
}

// flashlight
void util_turn_flashlight_on(void) {
    call_java("turn_flashlight_on", NULL);
}
void util_turn_flashlight_off(void) {
    call_java("turn_flashlight_off", NULL);
}
void util_toggle_flashlight(void) {
    call_java("toggle_flashlight", NULL);
}
bool util_is_flashlight_on(void) {
    return call_java("is_flashlight_on", NULL) == 1;
}

// playbackcapture
void util_start_playbackcapture(void) {
    call_java("start_playbackcapture", NULL);
}
void util_stop_playbackcapture(void) {
    call_java("stop_playbackcapture", NULL);
}

// -----------------  COMMON ROUTINE TO CALL JAVA METHOD  -------------------------

// returns:
// - INVALID_NUMBER, when failed, or
// - method specific result value, such as:
//   - latitude, longitude, or altitude
//   - 0 or 1 for boolean
//   - 0 for success
static double call_java(const char *method_name, char *arg_str)
{
    jmethodID method_id = 0;
    double method_ret_double = INVALID_NUMBER;

    // retrieve the JNI environment.,
    // retrieve the Java instance of the SDLActivity,
    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz(env->GetObjectClass(activity));

    // this routine supports these method signatures
    // - "()D" : double proc(void)
    // - "(Ljava/lang/String;)D" : double proc(java_lang_string)
    if (arg_str == NULL) {
        // get the method_id, print message if failed
        method_id = env->GetMethodID(clazz, method_name, "()D");

        // if got the method_id then call the start_foreground method
        if (method_id != 0) {
            method_ret_double = env->CallDoubleMethod(activity, method_id);
        }
    } else {
        // get the method_id, print message if failed
        method_id = env->GetMethodID(clazz, method_name, "(Ljava/lang/String;)D");

        // if got the method_id then ...
        if (method_id != 0) {
            // Convert C string to Java String
            // Note - When using JNI's NewStringUTF function, you are creating a new java.lang.String
            //        object within the Java Virtual Machine (JVM). This jstring is a local reference,
            //        and its memory management is handled by the JVM's garbage collector.
            jstring java_string = env->NewStringUTF(arg_str);

            // call text_to_speech method
            method_ret_double = env->CallDoubleMethod(activity, method_id, java_string);
        }
    }

    // print error messages
    if (method_id == 0) {
        ERROR("failed to get method_id for %s\n", method_name);
    } else if (method_ret_double == INVALID_NUMBER) {
        ERROR("%s method returned failure\n", method_name);
    }

    // clean up
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);

    // return method result
    return method_ret_double;
}

#else

// -----------------  NOT ANDROID - TEST CODE  ---------------------------

#include <utils.h>

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

void util_turn_flashlight_on(void) { }
void util_turn_flashlight_off(void) { }
void util_toggle_flashlight(void) { }
bool util_is_flashlight_on(void) { return false; }

void util_start_playbackcapture(void) { }
void util_stop_playbackcapture(void) { }

#endif
