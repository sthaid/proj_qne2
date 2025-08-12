#include <std_hdrs.h>

#include <sdl.h>
#include <utils.h>
#include <logging.h>

//
// defines
//

//
// typedefs
//

//
// variables
//

//
// prototypes
//

// -----------------  DEBUG & SUPPORT ROUTINES  -----------

static bool granted;
void cb(void *userdata, const char *permission, bool granted_arg)
{
    INFO("permission=%s  granted=%d\n", permission, granted_arg);
    granted = granted_arg;
}

void sdl_sensor_test(void)
{
    static bool first = true;
    bool succ;
    float data[6];
    static SDL_Sensor *step_counter;

    if (first) {
        sdl_sensor_print_devices();

        succ = SDL_RequestAndroidPermission("android.permission.ACTIVITY_RECOGNITION", cb, NULL);
        if (!succ) {
            ERROR("SDL_RequestAndroidPermission failed, %s\n", SDL_GetError());
        } else {
            while (!granted) {
                sleep(1);
            }
        }

        // step counter ID 15
        INFO("opening step counter ...\n");
        step_counter = SDL_OpenSensor(15);
        if (step_counter == NULL) {
            ERROR("failed to open step counter, %s\n", SDL_GetError());
        }
        first = false;
    }

    if (step_counter) {
        succ = SDL_GetSensorData(step_counter, data, 6);
        if (!succ) {
            ERROR("get step counter failed, %s\n", SDL_GetError());
            return;
        }
        INFO("step ctr = %f %f %f %f %f %f \n",
             data[0], data[1], data[2], data[3], data[4], data[5]);
    }

    //SDL_CloseSensor(step_counter);
}

void sdl_sensor_print_devices(void)
{
    int i, num_sensors = 0;
    SDL_SensorID *ids;
    //SDL_Sensor *sensor;
    const char *name;
    SDL_SensorType type;
    int nptype;

    ids = SDL_GetSensors(&num_sensors);
    if (ids == NULL) {
        ERROR("SDL_GetSensors returned NULL\n");
        return;
    }

    INFO("num_sensors = %d\n", num_sensors);
    for (i = 0; i < num_sensors; i++) {
        name = SDL_GetSensorNameForID(ids[i]);
        type = SDL_GetSensorTypeForID(ids[i]);
        nptype = SDL_GetSensorNonPortableTypeForID(ids[i]);
        INFO("  %d: %-60s type=%d  nptype=%d\n", ids[i], name, type, nptype);
    }

    SDL_free(ids);
}

#if 0
// xxx

// get list of sensors
SDL_GetSensors                     : SDL_SensorID * SDL_GetSensors(int *count);    // returns uint32[]

// get sensor names
SDL_GetSensorNameForID             : const char * SDL_GetSensorNameForID(SDL_SensorID instance_id);
SDL_GetSensorName                  : const char * SDL_GetSensorName(SDL_Sensor *sensor);

// convert SensorID to/from Sensor
SDL_GetSensorID                    : SDL_SensorID SDL_GetSensorID(SDL_Sensor *sensor)
SDL_GetSensorFromID                : SDL_Sensor * SDL_GetSensorFromID(SDL_SensorID instance_id);

// get type of sensor
SDL_GetSensorType                  : SDL_SensorType SDL_GetSensorType(SDL_Sensor *sensor);
SDL_GetSensorTypeForID             : SDL_SensorType SDL_GetSensorTypeForID(SDL_SensorID instance_id);

// open,close,read
SDL_OpenSensor                     : SDL_Sensor * SDL_OpenSensor(SDL_SensorID instance_id);
SDL_CloseSensor                    : void SDL_CloseSensor(SDL_Sensor *sensor);
SDL_GetSensorData                  : bool SDL_GetSensorData(SDL_Sensor *sensor, float *data, int num_values);
SDL_UpdateSensors                  : void SDL_UpdateSensors(void);   # updates the current state of the open sensors

// Android defined sensor types
SDL_GetSensorNonPortableType       : int SDL_GetSensorNonPortableType(SDL_Sensor *sensor);
SDL_GetSensorNonPortableTypeForID  : int SDL_GetSensorNonPortableTypeForID(SDL_SensorID instance_id);

SDL_GetSensorProperties            : SDL_PropertiesID SDL_GetSensorProperties(SDL_Sensor *sensor);    # ret is uint32

--- sensor types ---
https://wiki.libsdl.org/SDL3/SDL_SensorType

typedef enum SDL_SensorType {
    SDL_SENSOR_INVALID = -1,    /**< Returned for an invalid sensor */
    SDL_SENSOR_UNKNOWN,         /**< Unknown sensor type */
    SDL_SENSOR_ACCEL,           /**< Accelerometer */
    SDL_SENSOR_GYRO,            /**< Gyroscope */
    SDL_SENSOR_ACCEL_L,         /**< Accelerometer for left Joy-Con controller and Wii nunchuk */
    SDL_SENSOR_GYRO_L,          /**< Gyroscope for left Joy-Con controller */
    SDL_SENSOR_ACCEL_R,         /**< Accelerometer for right Joy-Con controller */
    SDL_SENSOR_GYRO_R           /**< Gyroscope for right Joy-Con controller */
} SDL_SensorType;

References:
- Sensor Events
    https://developer.android.com/reference/android/hardware/SensorEvent.html
    https://developer.android.com/reference/android/hardware/SensorEvent.html#values
- Sensor types
    https://source.android.com/docs/core/interaction/sensors/sensor-types
- SensorType Enum - non portable sensor types
    https://learn.microsoft.com/en-us/dotnet/api/android.hardware.sensortype?view=net-android-35.0
- Android SDK 
    ~/android_sdk/ndk/29.0.13846066/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/android/sensor.h
- Permissions
    https://developer.android.com/guide/topics/permissions/overview
    https://developer.android.com/reference/android/Manifest.permission
    SDL_RequestAndroidPermission

notes:
- wakeup sensor - wake the Application Processor from sleep state

---- Sensor Events ----

retrieved by SDL_PollEvent()   eventid = SDL_EVENT_SENSOR_UPDATE

typedef struct SDL_SensorEvent {
    SDL_EventType type; /**< SDL_EVENT_SENSOR_UPDATE */
    Uint32 reserved;
    Uint64 timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_SensorID which; /**< The instance ID of the sensor */
    float data[6];      /**< Up to 6 values from the sensor - 
                             additional values can be queried using SDL_GetSensorData() */
    Uint64 sensor_timestamp; /**< The timestamp of the sensor reading in nanoseconds, 
                                  not necessarily synchronized with the system clock */
} SDL_SensorEvent;


---- Sensor Types ----

typedef enum SDL_SensorType {
    SDL_SENSOR_INVALID = -1,    /**< Returned for an invalid sensor */
    SDL_SENSOR_UNKNOWN,         /**< Unknown sensor type */
    SDL_SENSOR_ACCEL,           /**< Accelerometer */
    SDL_SENSOR_GYRO,            /**< Gyroscope */
    SDL_SENSOR_ACCEL_L,         /**< Accelerometer for left Joy-Con controller and Wii nunchuk */
    SDL_SENSOR_GYRO_L,          /**< Gyroscope for left Joy-Con controller */
    SDL_SENSOR_ACCEL_R,         /**< Accelerometer for right Joy-Con controller */
    SDL_SENSOR_GYRO_R           /**< Gyroscope for right Joy-Con controller */
} SDL_SensorType;


---- List of Android Sensors on my device ----

num_sensors = 42
  2: lsm6dso LSM6DSO Accelerometer Non-wakeup                     type=1  nptype=1
  3: AK09918 Magnetometer                                         type=0  nptype=2
  4: lsm6dso LSM6DSO Gyroscope Non-wakeup                         type=2  nptype=4
  5: STK33911 Light  Non-wakeup                                   type=0  nptype=5
  6: lps22hh Pressure Sensor Non-wakeup                           type=0  nptype=6
  7: gravity  Non-wakeup                                          type=0  nptype=9
  8: linear_acceleration                                          type=0  nptype=10
  9: Rotation Vector  Non-wakeup                                  type=0  nptype=11
  10: AK09918 Magnetometer-Uncalibrated                            type=0  nptype=14
  11: Game Rotation Vector  Non-wakeup                             type=0  nptype=15
  12: lsm6dso LSM6DSO Gyroscope-Uncalibrated Non-wakeup            type=0  nptype=16
  13: smd  Wakeup                                                  type=0  nptype=17
  14: step_detector  Non-wakeup                                    type=0  nptype=18
  15: step_counter  Non-wakeup                                     type=0  nptype=19
  16: Tilt Detector  Wakeup                                        type=0  nptype=22
  17: Pick Up Gesture  Wakeup                                      type=0  nptype=25
  18: auto_rotation Screen Orientation Sensor Non-wakeup           type=0  nptype=27
  19: motion_detect                                                type=0  nptype=30
  20: lsm6dso LSM6DSO Accelerometer-Uncalibrated Non-wakeup        type=0  nptype=35
  21: STK33911 Light Strm WideIR Non-wakeup                        type=0  nptype=65578
  22: interrupt_gyro  Non-wakeup                                   type=0  nptype=65579
  23: STK33911 Proximity Strm  Non-wakeup                          type=0  nptype=65582
  24: SensorHub type                                               type=0  nptype=65586
  25: STK33911 Light Strm  Non-wakeup                              type=0  nptype=65587
  26: Wake Up Motion  Wakeup                                       type=0  nptype=65590
  27: STK33911 Proximity  Wakeup                                   type=0  nptype=65592
  28: call_gesture  Wakeup                                         type=0  nptype=65594
  29: STK33911 Auto Brightness Light  Non-wakeup                   type=0  nptype=65601
  30: Pocket mode  Wakeup                                          type=0  nptype=65605
  31: Led Cover Event  Wakeup                                      type=0  nptype=65606
  32: Light seamless  Wakeup                                       type=0  nptype=65614
  33: Flip Cover Detector  Wakeup                                  type=0  nptype=65639
  34: Sar BackOff Motion  Wakeup                                   type=0  nptype=65643
  35: Drop Classifier  Wakeup                                      type=0  nptype=65644
  36: Seq Step  Wakeup                                             type=0  nptype=65647
  37: Pocket Position Mode  Wakeup                                 type=0  nptype=65698
  38: TSL2585 Rear ALS                                             type=0  nptype=65577
  39: Touch Proximity Sensor                                       type=0  nptype=65596
  40: Hall IC                                                      type=0  nptype=65600
  41: Palm Proximity Sensor version 2                              type=0  nptype=8
  42: Motion Sensor                                                type=0  nptype=65559
  43: Orientation Sensor                                           type=0  nptype=3


#endif
