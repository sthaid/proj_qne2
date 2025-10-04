
// xxx add logging
// xxx update *.h for cpp

// This example requires C++ and a custom Java method named "void showHome()"

// xxx move these to some .h file
extern "C" {
void showHome(void);
void showHome2(void);
void get_altitude(void);
}

#ifdef ANDROID

#include <SDL3/SDL.h>
#include <jni.h>

void get_altitude(void)
{
    // retrieve the JNI environment.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "env = %p\n", env);

    // retrieve the Java instance of the SDLActivity
    jobject activity = (jobject)SDL_GetAndroidActivity();

    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    jclass clazz(env->GetObjectClass(activity));
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "clazz = %p\n", clazz);

    // find the identifier of the method to call
    jmethodID method_id = env->GetMethodID(clazz, "get_altitude", "()D");
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "method_id = %p\n", method_id);
    if (method_id == 0) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "ERROR method_id");
        goto cleanup;
    }

    // effectively call the Java method
    double result;
    result = env->CallDoubleMethod(activity, method_id);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "XXX result = %f", result);
    
cleanup:
    // clean up the local references.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);
}

// Calls the void showHome() method of the Java instance of the activity.
void showHome(void)
{
    // retrieve the JNI environment.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "env = %p\n", env);

    // retrieve the Java instance of the SDLActivity
    jobject activity = (jobject)SDL_GetAndroidActivity();

    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    jclass clazz(env->GetObjectClass(activity));
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "clazz = %p\n", clazz);

    // find the identifier of the method to call
    jmethodID method_id = env->GetMethodID(clazz, "showHome", "()V");
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "method_id = %p\n", method_id);
    if (method_id == 0) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "ERROR method_id");
        goto cleanup;
    }

    // effectively call the Java method
    env->CallVoidMethod(activity, method_id);

cleanup:
    // clean up the local references.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);

    // Warning (and discussion of implementation details of SDL for Android):
    // Local references are automatically deleted if a native function called
    // from Java side returns. For SDL this native function is main() itself.
    // Therefore references need to be manually deleted because otherwise the
    // references will first be cleaned if main() returns (application exit).
}

// Calls the void showHome2() method of the Java instance of the activity.
void showHome2(void)
{
    // retrieve the JNI environment.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "env = %p\n", env);

    // retrieve the Java instance of the SDLActivity
    jobject activity = (jobject)SDL_GetAndroidActivity();

    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    jclass clazz(env->GetObjectClass(activity));
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "clazz = %p\n", clazz);

    // find the identifier of the method to call
    jmethodID method_id = env->GetMethodID(clazz, "showHome2", "()V");
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "method_id = %p\n", method_id);
    if (method_id == 0) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "ERROR method_id");
        goto cleanup;
    }

    // effectively call the Java method
    env->CallVoidMethod(activity, method_id);

cleanup:
    // clean up the local references.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);

    // Warning (and discussion of implementation details of SDL for Android):
    // Local references are automatically deleted if a native function called
    // from Java side returns. For SDL this native function is main() itself.
    // Therefore references need to be manually deleted because otherwise the
    // references will first be cleaned if main() returns (application exit).
}

#else

void showHome(void)
{
}

void showHome2(void)
{
}

#endif
