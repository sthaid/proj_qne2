#include <std_hdrs.h>

#include <sdl.h>
#include <utils.h>
#include <logging.h>

// notes
// - sensor_tbl is indexed by SDL_SensorID, which assumes the 
//   SDL_SensorID is a small number, less than MAX_SENSOR_ID

//
// defines
//

#define MAX_SENSOR_ID 256

#define TEN_MS 10000

//
// typedefs
//

typedef struct {
    SDL_SensorType type;
    int            nptype;
    const char    *name;
    SDL_Sensor    *sensor;  // xxx is sensor opened as nptype or type?
    struct {
        double value;
    } data;
} sensor_t;

//
// variables
//

static sensor_t sensor_tbl[MAX_SENSOR_ID];

//
// prototypes
//

static int get_permission(char *name);

// -----------------  INIT & EVENT HANDLER  --------------

int sdl_sensor_init(void)
{
    int            i, num_sensors = 0;
    int            num_avail_sensors = 0;
    SDL_SensorID  *ids;

    // get list of sensor ids
    ids = SDL_GetSensors(&num_sensors);
    if (ids == NULL) {
        ERROR("SDL_GetSensors returned NULL\n");
        return -1;
    }

    // loop over all sensor ids, and save info in sensor_tbl
    for (i = 0; i < num_sensors; i++) {
        // check if sensor is device private
        if (SDL_GetSensorNonPortableTypeForID(ids[i]) >= 65536) {
            continue;
        }

        // check if id is out of range supported by this code
        if (ids[i] < 0 || ids[i] >= MAX_SENSOR_ID) {
            ERROR("sensor id %d is out of range, sensor ignored\n", ids[i]);
            continue;
        }

        // save sensor type, non-portable-type, and name in sensor_tbl
        sensor_tbl[ids[i]].type   = SDL_GetSensorTypeForID(ids[i]);
        sensor_tbl[ids[i]].nptype = SDL_GetSensorNonPortableTypeForID(ids[i]);
        sensor_tbl[ids[i]].name   = SDL_GetSensorNameForID(ids[i]);
        num_avail_sensors++;
    }

    // print the info from sensor_tbl
    INFO("num_sensors: total=%d avail=%d\n", num_sensors, num_avail_sensors);
    for (int id = 0; id < MAX_SENSOR_ID; id++) {
        if (sensor_tbl[id].name != NULL) {
            INFO("%2d %2d %2d %s\n",
                 id, sensor_tbl[id].type, sensor_tbl[id].nptype, sensor_tbl[id].name);
        }
    }

    // free the list of ids
    SDL_free(ids);

    // return success
    return 0;
}

// called from process_sdl_event, in sdl.c, when SDL_EVENT_SENSOR_UPDATE occurs
void sdl_sensor_event(SDL_SensorEvent *event)
{
    int id;
    sensor_t *sens;

    // validate sensor id is in range
    id = event->which;
    if (id < 0 || id >= MAX_SENSOR_ID) {
        ERROR("invlaid sensor id %d\n", id);
        return;
    }

    // validate sensor id is for an open sensor
    sens = &sensor_tbl[id];
    if (sens->sensor == NULL) {
        ERROR("sensor id %d is not open\n", id);
        return;
    }

    // xxx first check sensor type field ??

    // process the sensor data, save result in sensor_tbl[id].data struct
    switch (sens->nptype) {
    case ASENSOR_TYPE_STEP_COUNTER:
        sens->data.value++;
        break;
    default:
        ERROR("sensor id %d, invalid nptype %d\n", id, sens->nptype);
        break;
    }
}

// -----------  APIS AVAILABLE IN PICOC  --------------

int sdl_sensor_open(bool type_is_np, int type)
{
    int id, rc;
    SDL_Sensor *sdl_sensor;

    // get the id of the first sensor with the requested type/nptype;
    // note: 
    // - type   - sensors that are defined by SDL
    // - nptype - 'NonPortableType', values are platform dependant
    for (id = 0; id < MAX_SENSOR_ID; id++) {
        if (sensor_tbl[id].name == NULL) {
            continue;
        }
        if (type_is_np && type == sensor_tbl[id].nptype) {
            break;
        }
        if (!type_is_np && type == sensor_tbl[id].type) {
            break;
        }
    }
    if (id == MAX_SENSOR_ID) {
        ERROR("no sensor found for %s %d\n", (type_is_np ? "nptype" : "type"), type);
        return -1;
    }

    // if sensor id is already open then return error
    if (sensor_tbl[id].sensor != NULL) {
        ERROR("sensor id %d is already open\n", id);
        return -1;
    }

    // get permission, if required for the requested nptype; 
    // note that the permission may also be needed in AndroidManifest.xml
    if (type_is_np && type == ASENSOR_TYPE_STEP_COUNTER) {
        rc = get_permission("android.permission.ACTIVITY_RECOGNITION");
        if (rc < 0) {
            ERROR("failed to be granted ACTIVITY_RECOGNITION permission for STEP_COUNTER sensor\n");
            return -1;
        }
    }

    // clear sensor data
    memset(&sensor_tbl[id].data, 0, sizeof(sensor_tbl[id].data));

    // open the sensor
    sdl_sensor = SDL_OpenSensor(id);
    if (sdl_sensor == NULL) {
        ERROR("failed to open sensor id %d, %s\n", id, SDL_GetError());
        return -1;
    }
    sensor_tbl[id].sensor = sdl_sensor;
    INFO("sensor_tbl[%d].sensor = %p\n", id, sensor_tbl[id].sensor);

    // return sensor id
    return id;
}
        
void sdl_sensor_close(int id)
{
    if (id < 0 || id >= MAX_SENSOR_ID) {
        ERROR("id %d is out of range\n", id);
        return;
    }
    if (sensor_tbl[id].sensor == NULL) {
        ERROR("sensor %d is not open\n", id);
        return;
    }
    
    SDL_CloseSensor(sensor_tbl[id].sensor);
    sensor_tbl[id].sensor = NULL;
}

int sdl_sensor_read(int id, double *value)
{
    // preset return value
    *value = 0;

    // validate id arg is for an open sensor
    if (id < 0 || id >= MAX_SENSOR_ID) {
        ERROR("invlaid sensor id %d\n", id);
        return -1;
    }

    if (sensor_tbl[id].sensor == NULL) {
        ERROR("sensor id %d is not open\n", id);
        return -1;
    }

    // return sensor data
    *value = sensor_tbl[id].data.value;
    return 0;
}

// -----------------  PRIVATE  ----------------------------

#ifdef ANDROID

static void get_permission_cb(void *userdata, const char *permission, bool granted);

#define PERM_NO_RESULT    0
#define PERM_GRANTED      1
#define PERM_NOT_GRANTED  2

static int get_permission(char *name)
{
    bool succ;
    int perm_result;

    // request permission
    perm_result = PERM_NO_RESULT;
    succ = SDL_RequestAndroidPermission(name, get_permission_cb, &perm_result);
    if (!succ) {
        ERROR("SDL_RequestAndroidPermission failed, %s\n", SDL_GetError());
        return -1;
    }

    // wait for permission request to be either granted or not-granted
    while (perm_result == PERM_NO_RESULT) {
        usleep(TEN_MS);
    }

    // if not granted then return error
    if (perm_result != PERM_GRANTED) {
        ERROR("%s not granted\n", name);
        return -1;
    }

    // return success
    return 0;
}

static void get_permission_cb(void *userdata, const char *permission, bool granted)
{
    int *perm_result = (int*)userdata;

    INFO("permission=%s  granted=%d\n", permission, granted);
    *perm_result = (granted ? PERM_GRANTED : PERM_NOT_GRANTED);
}

#else

static int get_permission(char *name)
{
    return 0;
}

#endif

#if 0
// -----------------  NOTES  --------------------

------------
REFERENCES 
------------

- Sensor Events
    https://developer.android.com/reference/android/hardware/SensorEvent.html
    https://developer.android.com/reference/android/hardware/SensorEvent.html#values
- Sensor types
    https://source.android.com/docs/core/interaction/sensors/sensor-types
- SensorType Enum - non portable sensor types
    https://learn.microsoft.com/en-us/dotnet/api/android.hardware.sensortype?view=net-android-35.0
- Android SDK, NDK 29 sensor.h
    ~/android_sdk/ndk/29.0.13846066/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/android/sensor.h
- Permissions
    https://developer.android.com/guide/topics/permissions/overview
    https://developer.android.com/reference/android/Manifest.permission
    SDL_RequestAndroidPermission

---------------
SDL API
---------------

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

-------------
SENSOR TYPES
-------------

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

#define SDL_SENSOR_ACCEL    1
#define SDL_SENSOR_GYRO     2
#define SDL_SENSOR_ACCEL_L  3
#define SDL_SENSOR_GYRO_L   4
#define SDL_SENSOR_ACCEL_R  5
#define SDL_SENSOR_GYRO_R   6

--------------------------
SENSOR NON PORTABLE TYPES
--------------------------

From:
~/android_sdk/ndk/29.0.13846066/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/android/sensor.h

#define ASENSOR_TYPE_ACCELEROMETER       1
#define ASENSOR_TYPE_MAGNETIC_FIELD      2
#define ASENSOR_TYPE_GYROSCOPE           4
#define ASENSOR_TYPE_LIGHT               5
#define ASENSOR_TYPE_PRESSURE            6
#define ASENSOR_TYPE_PROXIMITY           8
#define ASENSOR_TYPE_GRAVITY             9
#define ASENSOR_TYPE_LINEAR_ACCELERATION 10
#define ASENSOR_TYPE_ROTATION_VECTOR     11
#define ASENSOR_TYPE_RELATIVE_HUMIDITY   12
#define ASENSOR_TYPE_AMBIENT_TEMPERATURE 13
#define ASENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED 14
#define ASENSOR_TYPE_GAME_ROTATION_VECTOR 15
#define ASENSOR_TYPE_GYROSCOPE_UNCALIBRATED 16
#define ASENSOR_TYPE_SIGNIFICANT_MOTION 17
#define ASENSOR_TYPE_STEP_DETECTOR 18
#define ASENSOR_TYPE_STEP_COUNTER 19
#define ASENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR 20
#define ASENSOR_TYPE_HEART_RATE 21
#define ASENSOR_TYPE_POSE_6DOF 28
#define ASENSOR_TYPE_STATIONARY_DETECT 29
#define ASENSOR_TYPE_MOTION_DETECT 30
#define ASENSOR_TYPE_HEART_BEAT 31
#define ASENSOR_TYPE_DYNAMIC_SENSOR_META 32
#define ASENSOR_TYPE_ADDITIONAL_INFO 33
#define ASENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT 34
#define ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED 35
#define ASENSOR_TYPE_HINGE_ANGLE 36
#define ASENSOR_TYPE_HEAD_TRACKER 37
#define ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES 38
#define ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES 39
#define ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES_UNCALIBRATED 40
#define ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES_UNCALIBRATED 41
#define ASENSOR_TYPE_HEADING 42

--------------
SENSOR EVENTS
--------------

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

----------
NOTES 
----------

- wakeup sensor - wake the Application Processor from sleep state

-----------------------------
ANDROID SENSORS ON MY DEVICE
-----------------------------

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
