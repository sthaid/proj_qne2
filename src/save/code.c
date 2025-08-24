static char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr)
{   
    char addr_str[100];
    int port2;
        
    if (addr->sa_family == AF_INET) {
        inet_ntop(AF_INET,
                  &((struct sockaddr_in*)addr)->sin_addr,
                  addr_str, sizeof(addr_str));
        port2 = ((struct sockaddr_in*)addr)->sin_port;
#if 0 //xxx
    } else if (addr->sa_family == AF_INET6) {
        inet_ntop(AF_INET6,
                  &((struct sockaddr_in6*)addr)->sin6_addr,
                 addr_str, sizeof(addr_str));
        port2 = ((struct sockaddr_in6*)addr)->sin6_port;
#endif
    } else {
        snprintf(s,slen,"Invalid AddrFamily %d", addr->sa_family);
        return s;
    }

    snprintf(s,slen,"%s:%d",addr_str,ntohs(port2));
    return s;
}

static void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0) {
        s[len-1] = '\0';
    }
}

//  rc = setpgid(0,0);
//  if (rc < 0) {
//      ERROR("setpgid failed, %s\n", strerror(errno));
//  }


if 0
// called from process_sdl_event, in sdl.c, when SDL_EVENT_SENSOR_UPDATE occurs
void sdl_sensor_event(SDL_SensorEvent *event)
{
    int id;
    sensor_t *sens;

    return; //xxx  maybe this wont be used

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
    // xxx can this be generalized
    switch (sens->nptype) {
    case ASENSOR_TYPE_STEP_COUNTER:
        sens->data.value++;
        break;
    default:
        ERROR("sensor id %d, invalid nptype %d\n", id, sens->nptype);
        break;
    }
}
#endif

