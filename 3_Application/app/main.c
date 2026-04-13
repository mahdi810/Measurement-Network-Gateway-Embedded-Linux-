#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <sys/socket.h>

#include "Ring_Buffer.h"
#include "sensor_read.h"

#define PORT         50012
#define CMD_BUF_SIZE 1024
#define PI           3.14159265358979323846
#define ServerPayloadSize	93


int simulation = 0;

/* ============================================================================
 * GLOBAL SHUTDOWN FLAG + SOCKET FDS
 * ========================================================================= */
static volatile int    g_shutdown      = 0;
static int             g_server_socket = -1;
static int             g_client_socket = -1;
static pthread_mutex_t g_socket_lock   = PTHREAD_MUTEX_INITIALIZER;

static void request_shutdown(void)
{
    g_shutdown = 1;

    pthread_mutex_lock(&g_socket_lock);
    if (g_client_socket >= 0) {
        shutdown(g_client_socket, SHUT_RDWR);
        close(g_client_socket);
        g_client_socket = -1;
    }
    if (g_server_socket >= 0) {
        shutdown(g_server_socket, SHUT_RDWR);
        close(g_server_socket);
        g_server_socket = -1;
    }
    pthread_mutex_unlock(&g_socket_lock);
}

/* ============================================================================
 * FAKE SENSOR DATA (simulation mode)
 * ========================================================================= */
typedef struct {
    double   temp_base;
    double   adc0_base;
    double   adc1_base;
    uint64_t startup_time;
} FakeSensorState;

static FakeSensorState fake_state = {
    .temp_base    = 25,
    .adc0_base    = 325,
    .adc1_base    = 545,
    .startup_time = 0
};

double GenerateFakeTemperature(uint64_t elapsed_us)
{
    (void)elapsed_us;
    return fake_state.temp_base;
}

uint8_t GenerateFakeSwitch(uint64_t elapsed_us)
{
    static uint64_t last_change = 0;
    static uint8_t  state = 0x0F;
    if (elapsed_us - last_change > 5000000ULL) {
        state = (state == 0x0F) ? 0x00 : 0x0F;
        last_change = elapsed_us;
    }
    return state;
}

uint8_t GenerateFakePushButton(void)
{
    return 100;
}

/* ============================================================================
 * THREAD STRUCTURES & GLOBALS
 * ========================================================================= */
typedef struct {
    unsigned int temp_config;
    unsigned int adc0_config;
    unsigned int adc1_config;
    unsigned int switch_config;
    unsigned int pushb_config;
} config_struct;

typedef struct {
    pthread_t       thobj;
    pthread_cond_t  thcond;
    pthread_mutex_t thmutex;
    config_struct   th_config;
    short int       thkill;
} thread_struct;

typedef enum {
    sid_temp     = 1,
    sid_adc0     = 2,
    sid_adc1     = 3,
    sid_switches = 4,
    sid_pushb    = 5
} sensor_id;

typedef struct {
    int      sid;
    int      svalue;
    uint64_t ts;
} packet;

typedef enum {
    STOPPED       = 0,
    RUNNING,
    PAUSED,
    CONFIGURATION,
    DISCONNECT,
    SHUTDOWN
} stream_state_t;

volatile stream_state_t stream_state;

thread_struct sensor_thread;
thread_struct server_thread;

config_struct main_config = {
    .temp_config   = 300,
    .adc0_config   = 300,
    .adc1_config   = 300,
    .switch_config = 300,
    .pushb_config  = 300
};

/* ============================================================================
 * SENSOR THREAD
 * ========================================================================= */
void *sensor_thread_function(void *arg)
{
    (void)arg;
    int TempInitializerFlag = 0;
    printf("Sensor thread started...\n");

    int fd = open("/dev/meascdd", O_RDWR);
    if (fd < 0) {
        simulation = 1;
        printf("Device open failed - simulation mode\n");
    } else {
        printf("Device opened successfully\n");
    }

    InitTimer();
    InitRingBuffer();

    if (!simulation) {
        if (TempPowerUp(fd) != 1) {
            printf("Temperature sensor init failed\n");
        } else {
            printf("Temperature sensor initialized\n");
            TempInitializerFlag = 1;
        }
    }

    unsigned int adc0_data, adc1_data, dummy0, dummy1;

    uint64_t next_temp_time   = GetElapsedTime();
    uint64_t next_adc0_time   = GetElapsedTime();
    uint64_t next_adc1_time   = GetElapsedTime();
    uint64_t next_switch_time = GetElapsedTime();
    uint64_t next_pushb_time  = GetElapsedTime();

    double t_adc0    = 0.0,  t_adc1    = 0.0;
    double freq_adc0 = 5.0,  freq_adc1 = 3.0;

    while (1)
    {
        pthread_mutex_lock(&sensor_thread.thmutex);
        int kill = sensor_thread.thkill;
        pthread_mutex_unlock(&sensor_thread.thmutex);
        if (kill || g_shutdown) break;

        // Recalculate enabled flags and periods on every iteration
        // based on current configuration
        pthread_mutex_lock(&server_thread.thmutex);
        
        int temp_enabled   = (main_config.temp_config > 0);
        int adc0_enabled   = (main_config.adc0_config > 0);
        int adc1_enabled   = (main_config.adc1_config > 0);
        int switch_enabled = (main_config.switch_config > 0);
        int pushb_enabled  = (main_config.pushb_config > 0);
        
        uint64_t period_temp_us   = (main_config.temp_config > 0) ? 1000000ULL / main_config.temp_config : 0;
        uint64_t period_adc0_us   = (main_config.adc0_config > 0) ? 1000000ULL / main_config.adc0_config : 0;
        uint64_t period_adc1_us   = (main_config.adc1_config > 0) ? 1000000ULL / main_config.adc1_config : 0;
        uint64_t period_switch_us = (main_config.switch_config > 0) ? 1000000ULL / main_config.switch_config : 0;
        uint64_t period_pushb_us  = (main_config.pushb_config > 0) ? 1000000ULL / main_config.pushb_config : 0;
        
        pthread_mutex_unlock(&server_thread.thmutex);

        uint64_t current_time = GetElapsedTime();

        // Calculate sleep target - only consider enabled sensors
        uint64_t sleep_target = UINT64_MAX;
        
        if (temp_enabled && next_temp_time < sleep_target) 
            sleep_target = next_temp_time;
        if (adc0_enabled && next_adc0_time < sleep_target) 
            sleep_target = next_adc0_time;
        if (adc1_enabled && next_adc1_time < sleep_target) 
            sleep_target = next_adc1_time;
        if (switch_enabled && next_switch_time < sleep_target) 
            sleep_target = next_switch_time;
        if (pushb_enabled && next_pushb_time < sleep_target) 
            sleep_target = next_pushb_time;

        // If all sensors are disabled, just sleep a bit and continue
        if (sleep_target == UINT64_MAX) {
            usleep(10000);  // Sleep 10ms and check again
            continue;
        }

        if (current_time < sleep_target) {
            uint64_t sleep_us = sleep_target - current_time;
            while (sleep_us > 0) {
                uint64_t chunk = (sleep_us > 100) ? 100 : sleep_us;
                usleep(chunk);
                sleep_us    -= chunk;
                current_time = GetElapsedTime();
                if (current_time >= sleep_target) break;
                if (g_shutdown) break;
            }
        }

        if (g_shutdown) break;

        /* ---- TEMPERATURE (only if enabled) ---- */
        if (temp_enabled && current_time >= next_temp_time) {
            int raw = 0;
            if (simulation) {
                raw = (int)(GenerateFakeTemperature(current_time) * 100);
                PushRB(sid_temp, current_time, (uint32_t)raw);
                printf("T=%d ", raw);
            } else {
                if (TempRead(fd, &raw) < 0) {
                    printf("T=FAULT ");
                } else {
                    PushRB(sid_temp, current_time, (uint32_t)raw);
                    printf("T=%d ", raw);
                }
            }
            next_temp_time += period_temp_us;
        }

        /* ---- ADC0 (only if enabled) ---- */
        if (adc0_enabled && current_time >= next_adc0_time) {
            if (simulation) {
                t_adc0   += 1.0 / main_config.adc0_config;
                adc0_data = (uint32_t)(fake_state.adc0_base +
                            100.0 * sin(2.0 * PI * freq_adc0 * t_adc0));
            } else {
                GetADC(fd, &adc0_data, &dummy0);
            }
            PushRB(sid_adc0, current_time, adc0_data);
            printf("A0=%u ", adc0_data);
            next_adc0_time += period_adc0_us;
        }

        /* ---- ADC1 (only if enabled) ---- */
        if (adc1_enabled && current_time >= next_adc1_time) {
            if (simulation) {
                t_adc1   += 1.0 / main_config.adc1_config;
                adc1_data = (uint32_t)(fake_state.adc1_base +
                            150.0 * cos(2.0 * PI * freq_adc1 * t_adc1));
            } else {
                GetADC(fd, &dummy1, &adc1_data);
            }
            PushRB(sid_adc1, current_time, adc1_data);
            printf("A1=%u ", adc1_data);
            next_adc1_time += period_adc1_us;
        }

        /* ---- SWITCH (only if enabled) ---- */
        if (switch_enabled && current_time >= next_switch_time) {
            uint8_t sw = simulation ? GenerateFakeSwitch(current_time): ReadSwitch(fd);
            uint8_t LastSwitchState = 0xff;

            if(!simulation && sw != LastSwitchState){
            	LedWrite(fd, sw);
            	LastSwitchState = sw;
            }

            PushRB(sid_switches, current_time, sw);
            printf("SW=%u ", sw);
            next_switch_time += period_switch_us;
        }

        /* ---- PUSH BUTTON (only if enabled) ---- */
        if (pushb_enabled && current_time >= next_pushb_time) {
            uint8_t pb = simulation ? GenerateFakePushButton()
                                    : ReadButton(fd);
            PushRB(sid_pushb, current_time, pb);
            printf("PB=%u ", pb);
            next_pushb_time += period_pushb_us;
        }

        if (temp_enabled || adc0_enabled || adc1_enabled || switch_enabled || pushb_enabled) {
            printf("\n");
        }
    }

    if (!simulation) TempShutdown(fd);
    if (fd >= 0) close(fd);
    (void)TempInitializerFlag;
    printf("Sensor thread exiting\n");
    return NULL;
}

/* ============================================================================
 * HELPERS
 * ========================================================================= */
void set_config(config_struct *config)
{
    pthread_mutex_lock(&server_thread.thmutex);
    main_config.temp_config   = config->temp_config;
    main_config.adc0_config   = config->adc0_config;
    main_config.adc1_config   = config->adc1_config;
    main_config.switch_config = config->switch_config;
    main_config.pushb_config  = config->pushb_config;
    pthread_mutex_unlock(&server_thread.thmutex);
}

void print_config(void)
{
    printf("temp_config:     %u\n", main_config.temp_config);
    printf("adc0_config:     %u\n", main_config.adc0_config);
    printf("adc1_config:     %u\n", main_config.adc1_config);
    printf("switches_config: %u\n", main_config.switch_config);
    printf("pushb_config:    %u\n", main_config.pushb_config);
}

/* ============================================================================
 * SERVER THREAD
 * ========================================================================= */
void *server_thread_function(void *arg)
{
    (void)arg;
    packet payload[ServerPayloadSize];
    char   receive_command[CMD_BUF_SIZE];

    struct sockaddr_in server_address, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    stream_state = PAUSED;

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) { perror("socket"); return NULL; }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family      = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port        = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0) {
        perror("bind"); close(server_sock); return NULL;
    }
    if (listen(server_sock, 1) < 0) {
        perror("listen"); close(server_sock); return NULL;
    }

    pthread_mutex_lock(&g_socket_lock);
    g_server_socket = server_sock;
    pthread_mutex_unlock(&g_socket_lock);

    printf("Server listening on port %d\n", PORT);

    /* ---- OUTER LOOP: wait for a client ---- */
    while (!g_shutdown)
    {
        printf("Waiting for client...\n");

        int client_sock = accept(server_sock,
                                 (struct sockaddr *)&client_addr,
                                 &client_addr_len);
        if (client_sock < 0) {
            if (g_shutdown) break;
            perror("accept");
            continue;
        }

        pthread_mutex_lock(&g_socket_lock);
        g_client_socket = client_sock;
        pthread_mutex_unlock(&g_socket_lock);

        printf("Client connected\n");
        stream_state = PAUSED;

        /* ---- INNER LOOP: serve one client ---- */
        while (!g_shutdown)
        {
            memset(receive_command, 0, CMD_BUF_SIZE);
            int n = recv(client_sock,
                         receive_command,
                         CMD_BUF_SIZE - 1,
                         MSG_DONTWAIT);

            if (n > 0)
            {
                receive_command[n] = '\0';
                for (char *p = receive_command; *p; p++)
                    if (*p == '\n' || *p == '\r') { *p = '\0'; break; }

                printf("Command: [%s]\n", receive_command);

                /* ---- start ---- */
                if (!strcmp(receive_command, "start"))
                {
                    int available;
                    pthread_mutex_lock(&server_thread.thmutex);
                    available = Ring_Buf.rb_cnt;
                    pthread_mutex_unlock(&server_thread.thmutex);

                    if (available > 0) {
                        int to_send = (available >= ServerPayloadSize) ? ServerPayloadSize : available;
                        for (int i = 0; i < to_send; i++) {
                            if (PopRB(&payload[i].sid,
                                      &payload[i].ts,
                                      &payload[i].svalue) < 0) {
                                to_send = i;
                                break;
                            }
                        }
                        if (to_send > 0) {
                            ssize_t sent = send(client_sock, payload,
                                                to_send * sizeof(packet), 0);
                            if (sent <= 0) { perror("send"); break; }
                            printf("Sent %d packets\n", to_send);
                        }
                    } else {
                        printf("start: ring buffer empty\n");
                    }
                    stream_state = PAUSED;
                }

                /* ---- pause / stop ---- */
                else if (!strcmp(receive_command, "pause") ||
                         !strcmp(receive_command, "stop"))
                {
                    stream_state = PAUSED;
                    printf("%s acknowledged\n", receive_command);
                }

                /* ---- configure ---- */
                else if (!strncmp(receive_command, "configure ", 10))
                {
                    config_struct cfg;
                    int parsed = sscanf(&receive_command[10],
                                        "%u %u %u %u %u",
                                        &cfg.temp_config,
                                        &cfg.adc0_config,
                                        &cfg.adc1_config,
                                        &cfg.switch_config,
                                        &cfg.pushb_config);
                    if (parsed == 5) { set_config(&cfg); print_config(); }
                    else printf("Bad configure arguments\n");
                }

                /* ---- disconnect ---- */
                else if (!strcmp(receive_command, "disconnect"))
                {
                    printf("Client requested disconnect\n");
                    stream_state = DISCONNECT;
                }

                /* ---- shutdown ---- */
                else if (!strcmp(receive_command, "shutdown"))
                {
                    printf("Shutdown command received - stopping server\n");
                    stream_state = SHUTDOWN;

                    shutdown(client_sock, SHUT_RDWR);
                    close(client_sock);

                    pthread_mutex_lock(&g_socket_lock);
                    g_client_socket = -1;
                    pthread_mutex_unlock(&g_socket_lock);

                    request_shutdown();

                    pthread_mutex_lock(&sensor_thread.thmutex);
                    sensor_thread.thkill = 1;
                    pthread_mutex_unlock(&sensor_thread.thmutex);

                    break;
                }

                else
                {
                    printf("Unknown command: [%s]\n", receive_command);
                }
            }
            else if (n == 0)
            {
                printf("Client disconnected\n");
                break;
            }
            else
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                    perror("recv");
                    break;
                }
            }

            if (stream_state == DISCONNECT) {
                printf("Closing client connection\n");
                shutdown(client_sock, SHUT_RDWR);
                close(client_sock);

                pthread_mutex_lock(&g_socket_lock);
                g_client_socket = -1;
                pthread_mutex_unlock(&g_socket_lock);
                break;
            }

            usleep(1000);
        } /* inner loop */

        pthread_mutex_lock(&g_socket_lock);
        if (g_client_socket >= 0) {
            close(g_client_socket);
            g_client_socket = -1;
        }
        pthread_mutex_unlock(&g_socket_lock);

    } /* outer loop */

    pthread_mutex_lock(&g_socket_lock);
    if (g_server_socket >= 0) {
        close(g_server_socket);
        g_server_socket = -1;
    }
    pthread_mutex_unlock(&g_socket_lock);

    printf("Server thread exiting\n");
    return NULL;
}

/* ============================================================================
 * MAIN
 * ========================================================================= */
int main(void)
{
    printf("mng main running...\n");

    pthread_mutex_init(&server_thread.thmutex, NULL);
    pthread_mutex_init(&sensor_thread.thmutex, NULL);
    sensor_thread.thkill = 0;

    print_config();

    int rc;

    rc = pthread_create(&sensor_thread.thobj, NULL,
                        sensor_thread_function, NULL);
    if (rc != 0) {
        fprintf(stderr, "sensor thread failed: %s\n", strerror(rc));
        return 1;
    }
    printf("Sensor thread created\n");

    rc = pthread_create(&server_thread.thobj, NULL,
                        server_thread_function, NULL);
    if (rc != 0) {
        fprintf(stderr, "server thread failed: %s\n", strerror(rc));
        return 1;
    }
    printf("Server thread created\n");

    pthread_join(server_thread.thobj, NULL);
    pthread_join(sensor_thread.thobj, NULL);

    pthread_mutex_destroy(&sensor_thread.thmutex);
    pthread_mutex_destroy(&server_thread.thmutex);
    pthread_mutex_destroy(&g_socket_lock);

    printf("Program terminated cleanly\n");
    return 0;
}
