#include <stddef.h>

#include <utils.h>
#include <logging.h>

#define INVALID_NUMBER 999999999  // get this from sdlx.h

#ifdef ANDROID

#include <SDL3/SDL.h>
#include <jni.h>

// The following comment is copied from here:
//   https://wiki.libsdl.org/SDL3/SDL_GetAndroidActivity
//
// Warning (and discussion of implementation details of SDL for Android):
// Local references are automatically deleted if a native function called
// from Java side returns. For SDL this native function is main() itself.
// Therefore references need to be manually deleted because otherwise the
// references will first be cleaned if main() returns (application exit).

void util_get_location(double *latitude, double *longitude, double *altitude)
{
    jmethodID method_id;

    INFO("XXX TEST INFO PRINT FROM CPP\n");

    *latitude = INVALID_NUMBER;
    *longitude = INVALID_NUMBER;
    *altitude = INVALID_NUMBER;

    // retrieve the JNI environment.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();

    // retrieve the Java instance of the SDLActivity
    jobject activity = (jobject)SDL_GetAndroidActivity();

    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    jclass clazz(env->GetObjectClass(activity));

    // get the method_id and call the methods to get location information
    // xxx clean this up
    method_id = env->GetMethodID(clazz, "get_latitude", "()D");
    if (method_id != 0 && latitude != NULL) {
        *latitude = env->CallDoubleMethod(activity, method_id);
    }
    method_id = env->GetMethodID(clazz, "get_longitude", "()D");
    if (method_id != 0 && longitude != NULL) {
        *longitude = env->CallDoubleMethod(activity, method_id);
    }
    method_id = env->GetMethodID(clazz, "get_altitude", "()D");
    if (method_id != 0 && altitude != NULL) {
        *altitude = env->CallDoubleMethod(activity, method_id);
    }

cleanup:
    // clean up the localreferences.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);
}

#else

#define BOLTON_MASS_LATITUDE     42.4334
#define BOLTON_MASS_LONGITUDE   -71.6078
#define BOLTON_MASS_ELEVATION    100    // range is 63 to 201 meters

// xxx google AI: does android provide gps altitude in feet or meters
// - says meters
// - accuracy 10 - 20 meters

// xxx rename this to get_gps_location
void util_get_location(double *latitude, double *longitude, double *altitude)
{
    if (latitude != NULL) {
        *latitude = BOLTON_MASS_LATITUDE;
    }
    if (longitude != NULL) {
        *longitude = BOLTON_MASS_LONGITUDE;
    }
    if (altitude != NULL) {
        *altitude = BOLTON_MASS_ELEVATION;
    }
}

#endif
