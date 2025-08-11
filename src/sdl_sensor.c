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

void sdl_sensor_test(void)
{
    sdl_sensor_print_devices();
}

void sdl_sensor_print_devices(void)
{
    int i, num_sensors = 0;
    SDL_SensorID *ids;
    //SDL_Sensor *sensor;
    const char *name;
    SDL_SensorType type;

    ids = SDL_GetSensors(&num_sensors);
    if (ids == NULL) {
        ERROR("SDL_GetSensors returned NULL\n");
        return;
    }

    INFO("num_sensors = %d\n", num_sensors);
    for (i = 0; i < num_sensors; i++) {
        name = SDL_GetSensorNameForID(ids[i]);
        type = SDL_GetSensorTypeForID(ids[i]);
        INFO("  %d: %-16s %d\n", ids[i], name, type);
    }

    SDL_free(ids);
}

#if 0
// get list of sensors
SDL_GetSensors                     : SDL_SensorID * SDL_GetSensors(int *count);    // returns uint32

// get sensor names
SDL_GetSensorNameForID             : const char * SDL_GetSensorNameForID(SDL_SensorID instance_id);
SDL_GetSensorName                  : const char * SDL_GetSensorName(SDL_Sensor *sensor);

// convert SensorID to/from Sensor
SDL_GetSensorID                    : SDL_SensorID SDL_GetSensorID(SDL_Sensor *sensor)
SDL_GetSensorFromID                : SDL_Sensor * SDL_GetSensorFromID(SDL_SensorID instance_id);

// get type of sensor
SDL_GetSensorType                  : SDL_SensorType SDL_GetSensorType(SDL_Sensor *sensor);
SDL_GetSensorTypeForID             : SDL_SensorType SDL_GetSensorTypeForID(SDL_SensorID instance_id);

// xxx
SDL_GetSensorData
SDL_GetSensorNonPortableType
SDL_GetSensorNonPortableTypeForID
SDL_GetSensorProperties

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

additional Android sensors ...
https://developer.android.com/reference/android/hardware/SensorEvent.html#values

------------------

num_sensors = 42
  2: lsm6dso LSM6DSO Accelerometer Non-wakeup 1
  3: AK09918 Magnetometer 0
  4: lsm6dso LSM6DSO Gyroscope Non-wakeup 2
  5: STK33911 Light  Non-wakeup 0
  6: lps22hh Pressure Sensor Non-wakeup 0
  7: gravity  Non-wakeup 0
  8: linear_acceleration 0
  9: Rotation Vector  Non-wakeup 0
  10: AK09918 Magnetometer-Uncalibrated 0
  11: Game Rotation Vector  Non-wakeup 0
  12: lsm6dso LSM6DSO Gyroscope-Uncalibrated Non-wakeup 0
  13: smd  Wakeup      0
  14: step_detector  Non-wakeup 0
  15: step_counter  Non-wakeup 0
  16: Tilt Detector  Wakeup 0
  17: Pick Up Gesture  Wakeup 0
  18: auto_rotation Screen Orientation Sensor Non-wakeup 0
  19: motion_detect    0
  20: lsm6dso LSM6DSO Accelerometer-Uncalibrated Non-wakeup 0
  21: STK33911 Light Strm WideIR Non-wakeup 0
  22: interrupt_gyro  Non-wakeup 0
  23: STK33911 Proximity Strm  Non-wakeup 0
  24: SensorHub type   0
  25: STK33911 Light Strm  Non-wakeup 0
  26: Wake Up Motion  Wakeup 0
  27: STK33911 Proximity  Wakeup 0
  28: call_gesture  Wakeup 0
  29: STK33911 Auto Brightness Light  Non-wakeup 0
  30: Pocket mode  Wakeup 0
  31: Led Cover Event  Wakeup 0
  32: Light seamless  Wakeup 0
  33: Flip Cover Detector  Wakeup 0
  34: Sar BackOff Motion  Wakeup 0
  35: Drop Classifier  Wakeup 0
  36: Seq Step  Wakeup 0
  37: Pocket Position Mode  Wakeup 0
  38: TSL2585 Rear ALS 0
  39: Touch Proximity Sensor 0
  40: Hall IC          0
  41: Palm Proximity Sensor version 2 0
  42: Motion Sensor    0
  43: Orientation Sensor 0

#endif
