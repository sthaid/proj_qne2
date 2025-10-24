//#include <stddef.h>

#include <utils.h>
#include <logging.h>

#define INVALID_NUMBER 999999999  // xxx get this from sdlx.h; or fix picoc so NAN/isnan works

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

    INFO("XXX TEST INFO PRINT FROM CPP\n");

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

    // xxx retry if results are 0.0

cleanup:
    // clean up the localreferences.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);
}

#else

// unit test version returns my town location

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

#endif
