#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uhid.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define GOIO_PACKET_SIZE 8
#define GOIO_MAX_RESPONSE_PACKETS 4
#define GOIO_DEFAULT_PERIOD_MS 100
#define GOIO_MIN_PERIOD_MS 5

struct packet_set {
    uint8_t packets[GOIO_MAX_RESPONSE_PACKETS][GOIO_PACKET_SIZE];
    size_t count;
    bool starts_measurements;
    bool stops_measurements;
};

struct simulator {
    int fd;
    bool measuring;
    uint8_t rolling_counter;
    uint8_t analog_channel;
    uint32_t period_ms;
    uint64_t next_measurement_ms;
    uint32_t sample_index;
};

static volatile sig_atomic_t running = 1;

static void stop_running(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static uint64_t monotonic_ms(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return 0;
    return (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000;
}

static uint32_t read_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static void write_le32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static void make_default_response(struct packet_set *set, uint8_t command)
{
    uint8_t *packet = set->packets[0];

    memset(packet, 0, GOIO_PACKET_SIZE);
    packet[0] = 0x5a;
    packet[1] = command;
    set->count = 1;
}

static void make_error_response(struct packet_set *set, uint8_t command, uint8_t status)
{
    make_default_response(set, command);
    set->packets[0][0] = 0x7a;
    set->packets[0][2] = status;
}

static void build_flash_record(uint8_t record[20])
{
    memset(record, 0, 20);
    record[0] = 1;       /* record version */
    record[1] = 0x9c;    /* valid Go!Link flash signature */
    record[2] = 1;       /* week, BCD */
    record[3] = 0x26;    /* year, BCD */
    record[8] = 0x3f;    /* vinSlope: big-endian 1.0f */
    record[9] = 0x80;
    record[12] = 0x3f;   /* vinLowSlope: big-endian 1.0f */
    record[13] = 0x80;
}

static bool append_nv_read_packets(struct packet_set *set, uint8_t command,
                                   uint8_t address, uint8_t count)
{
    uint8_t record[20];
    size_t copied = 0;
    bool first = true;

    if (count == 0 || address >= sizeof(record) ||
        (size_t)address + count > sizeof(record))
        return false;

    build_flash_record(record);
    while (copied < count) {
        uint8_t *packet;
        size_t chunk = count - copied;
        bool last;
        uint8_t header;

        if (set->count >= GOIO_MAX_RESPONSE_PACKETS)
            return false;
        if (chunk > (first ? 6u : 7u))
            chunk = first ? 6u : 7u;
        last = copied + chunk == count;
        header = 0x40u | (first ? 0x08u : 0u) | (last ? 0x10u : 0u) |
                 (uint8_t)(chunk + (first ? 1u : 0u));
        packet = set->packets[set->count++];
        memset(packet, 0, GOIO_PACKET_SIZE);
        packet[0] = header;
        if (first) {
            packet[1] = command;
            memcpy(packet + 2, record + address + copied, chunk);
        } else {
            memcpy(packet + 1, record + address + copied, chunk);
        }
        copied += chunk;
        first = false;
    }
    return true;
}

static void build_command_responses(const uint8_t command_packet[GOIO_PACKET_SIZE],
                                    struct simulator *simulator,
                                    struct packet_set *set)
{
    const uint8_t command = command_packet[0];
    uint8_t *response;

    memset(set, 0, sizeof(*set));
    switch (command) {
    case 0x10: /* GET_STATUS */
        response = set->packets[0];
        memset(response, 0, GOIO_PACKET_SIZE);
        response[0] = 0x5e;
        response[1] = command;
        response[3] = 0x01;
        response[4] = 0x01;
        response[5] = 0x01;
        response[6] = 0x01;
        set->count = 1;
        break;
    case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16:
    case 0x18: /* START_MEASUREMENTS */
        make_default_response(set, command);
        if (command == 0x18)
            set->starts_measurements = true;
        break;
    case 0x17: /* READ_LOCAL_NV_MEM */
        if (!append_nv_read_packets(set, command, command_packet[1], command_packet[2]))
            make_error_response(set, command, 0x36);
        break;
    case 0x19: /* STOP_MEASUREMENTS */
        make_default_response(set, command);
        set->stops_measurements = true;
        break;
    case 0x1a: /* INIT */
        response = set->packets[0];
        memset(response, 0, GOIO_PACKET_SIZE);
        response[0] = 0x9a;
        response[1] = command;
        set->count = 1;
        set->stops_measurements = true;
        break;
    case 0x1b: /* SET_MEASUREMENT_PERIOD */
        simulator->period_ms = read_le32(command_packet + 1);
        if (simulator->period_ms < GOIO_MIN_PERIOD_MS)
            simulator->period_ms = GOIO_MIN_PERIOD_MS;
        make_default_response(set, command);
        break;
    case 0x1c: /* GET_MEASUREMENT_PERIOD */
        response = set->packets[0];
        memset(response, 0, GOIO_PACKET_SIZE);
        response[0] = 0x5d;
        response[1] = command;
        write_le32(response + 2, simulator->period_ms);
        set->count = 1;
        break;
    case 0x1d: /* SET_LED_STATE */
        make_default_response(set, command);
        break;
    case 0x1e: /* GET_LED_STATE */
        response = set->packets[0];
        memset(response, 0, GOIO_PACKET_SIZE);
        response[0] = 0x5b;
        response[1] = command;
        response[2] = 0x80;
        response[3] = 0x04;
        set->count = 1;
        break;
    case 0x20: /* GET_SERIAL_NUMBER */
        response = set->packets[0];
        memset(response, 0, GOIO_PACKET_SIZE);
        response[0] = 0x6f;
        response[1] = command;
        response[2] = 0x01;
        response[3] = 0x26;
        response[4] = 0x01;
        response[5] = 0x00;
        response[6] = 0x00;
        response[7] = 0x00;
        set->count = 1;
        break;
    case 0x28: /* GET_SENSOR_ID: generic analog voltage probe (ID 2) */
        response = set->packets[0];
        memset(response, 0, GOIO_PACKET_SIZE);
        response[0] = 0x5f;
        response[1] = command;
        response[2] = 2;
        set->count = 1;
        break;
    case 0x29: /* SET_ANALOG_INPUT_CHANNEL */
        simulator->analog_channel = command_packet[1];
        make_default_response(set, command);
        break;
    case 0x2a: /* GET_ANALOG_INPUT_CHANNEL */
        response = set->packets[0];
        memset(response, 0, GOIO_PACKET_SIZE);
        response[0] = 0x5a;
        response[1] = command;
        response[2] = simulator->analog_channel;
        set->count = 1;
        break;
    case 0x2b: /* GET_VIN_OFFSET_DAC */
        response = set->packets[0];
        memset(response, 0, GOIO_PACKET_SIZE);
        response[0] = 0x5a;
        response[1] = command;
        set->count = 1;
        break;
    case 0x1f: /* SET_VIN_OFFSET_DAC */
    case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26:
        make_default_response(set, command);
        break;
    case 0xd0: /* internal poll: no host-visible command response */
        break;
    default:
        make_error_response(set, command, 0x31);
        break;
    }
}

static int write_uhid_event(int fd, const struct uhid_event *event)
{
    ssize_t written;

    do {
        written = write(fd, event, sizeof(*event));
    } while (written < 0 && errno == EINTR);
    if (written != (ssize_t)sizeof(*event)) {
        if (written < 0)
            perror("write /dev/uhid");
        else
            fprintf(stderr, "short write to /dev/uhid (%zd bytes)\n", written);
        return -1;
    }
    return 0;
}

static int send_input_packet(struct simulator *simulator,
                             const uint8_t packet[GOIO_PACKET_SIZE])
{
    struct uhid_event event;

    memset(&event, 0, sizeof(event));
    event.type = UHID_INPUT2;
    event.u.input2.size = GOIO_PACKET_SIZE;
    memcpy(event.u.input2.data, packet, GOIO_PACKET_SIZE);
    return write_uhid_event(simulator->fd, &event);
}

static int send_response_packets(struct simulator *simulator,
                                 const struct packet_set *set)
{
    size_t i;

    for (i = 0; i < set->count; ++i) {
        if (send_input_packet(simulator, set->packets[i]) != 0)
            return -1;
        printf("  IN");
        for (size_t j = 0; j < GOIO_PACKET_SIZE; ++j)
            printf(" %02x", set->packets[i][j]);
        putchar('\n');
    }
    if (set->stops_measurements) {
        simulator->measuring = false;
        simulator->next_measurement_ms = 0;
    }
    if (set->starts_measurements) {
        simulator->measuring = true;
        simulator->next_measurement_ms = monotonic_ms() + simulator->period_ms;
    }
    fflush(stdout);
    return 0;
}

static int process_command(struct simulator *simulator, const uint8_t *data,
                           size_t size)
{
    uint8_t command_packet[GOIO_PACKET_SIZE] = {0};
    struct packet_set responses;
    size_t offset = 0;

    if (size >= GOIO_PACKET_SIZE + 1 && data[0] == 0)
        offset = 1; /* Windows HID report ID prefix */
    if (size < offset + GOIO_PACKET_SIZE) {
        fprintf(stderr, "ignored short output report (%zu bytes)\n", size);
        return 0;
    }
    memcpy(command_packet, data + offset, GOIO_PACKET_SIZE);
    printf("OUT cmd=%02x params=", command_packet[0]);
    for (size_t i = 1; i < GOIO_PACKET_SIZE; ++i)
        printf("%s%02x", i == 1 ? "" : " ", command_packet[i]);
    putchar('\n');

    build_command_responses(command_packet, simulator, &responses);
    return send_response_packets(simulator, &responses);
}

static int reply_get_report(struct simulator *simulator, uint32_t request_id)
{
    struct uhid_event reply;

    memset(&reply, 0, sizeof(reply));
    reply.type = UHID_GET_REPORT_REPLY;
    reply.u.get_report_reply.id = request_id;
    reply.u.get_report_reply.err = 0;
    reply.u.get_report_reply.size = GOIO_PACKET_SIZE;
    return write_uhid_event(simulator->fd, &reply);
}

static int reply_set_report(struct simulator *simulator, uint32_t request_id)
{
    struct uhid_event reply;

    memset(&reply, 0, sizeof(reply));
    reply.type = UHID_SET_REPORT_REPLY;
    reply.u.set_report_reply.id = request_id;
    reply.u.set_report_reply.err = 0;
    return write_uhid_event(simulator->fd, &reply);
}

static uint16_t measurement_value(uint32_t sample_index)
{
    return (uint16_t)(8192 + (sample_index % 200) * 32);
}

static void build_measurement_packet(uint32_t sample_index, uint8_t rolling_counter,
                                     uint8_t packet[GOIO_PACKET_SIZE])
{
    uint16_t value = measurement_value(sample_index);

    memset(packet, 0, GOIO_PACKET_SIZE);
    packet[0] = 1; /* one measurement in this report */
    packet[1] = rolling_counter;
    packet[2] = (uint8_t)value;
    packet[3] = (uint8_t)(value >> 8);
}

static int send_measurement(struct simulator *simulator)
{
    uint8_t packet[GOIO_PACKET_SIZE] = {0};
    uint16_t value = measurement_value(simulator->sample_index);
    int result;

    build_measurement_packet(simulator->sample_index, simulator->rolling_counter++, packet);
    result = send_input_packet(simulator, packet);
    if (result == 0 && simulator->sample_index % 20 == 0) {
        printf("MEAS sample=%u raw=%u rolling=%u\n", simulator->sample_index,
               value, packet[1]);
        fflush(stdout);
    }
    ++simulator->sample_index;
    simulator->next_measurement_ms = monotonic_ms() + simulator->period_ms;
    return result;
}

static int run_device(void)
{
    static const uint8_t report_descriptor[] = {
        0x06, 0x00, 0xff, 0x09, 0x01, 0xa1, 0x01,
        0x09, 0x02, 0x15, 0x00, 0x26, 0xff, 0x00,
        0x75, 0x08, 0x95, 0x08, 0x81, 0x02,
        0x09, 0x03, 0x95, 0x08, 0x91, 0x02,
        0x09, 0x04, 0x95, 0x08, 0xb1, 0x02, 0xc0
    };
    struct simulator simulator = {
        .fd = -1,
        .measuring = false,
        .rolling_counter = 0,
        .analog_channel = 1,
        .period_ms = GOIO_DEFAULT_PERIOD_MS,
        .next_measurement_ms = 0,
        .sample_index = 0,
    };
    struct uhid_event event;
    int ret = 1;

    simulator.fd = open("/dev/uhid",  O_RDWR | O_CLOEXEC);
    if (simulator.fd < 0) {
        perror("open /dev/uhid");
        return 1;
    }

    memset(&event, 0, sizeof(event));
    event.type = UHID_CREATE2;
    snprintf((char *)event.u.create2.name, sizeof(event.u.create2.name),
             "Vernier Go!Link Protocol Test Emulator");
    snprintf((char *)event.u.create2.phys, sizeof(event.u.create2.phys),
             "uhid/virtual/loggerpro-golink");
    snprintf((char *)event.u.create2.uniq, sizeof(event.u.create2.uniq),
             "loggerpro-golink-protocol-test");
    event.u.create2.rd_size = sizeof(report_descriptor);
    event.u.create2.bus = BUS_USB;
    event.u.create2.vendor = 0x08f7;
    event.u.create2.product = 0x0003;
    event.u.create2.version = 0x0100;
    memcpy(event.u.create2.rd_data, report_descriptor, sizeof(report_descriptor));
    if (write_uhid_event(simulator.fd, &event) != 0) {
        close(simulator.fd);
        return 1;
    }

    signal(SIGINT, stop_running);
    signal(SIGTERM, stop_running);
    puts("Created protocol-aware virtual Go!Link HID 08f7:0003.");
    puts("This emits synthetic voltage samples; it is not a real sensor.");
    puts("Press Ctrl-C to remove the virtual device.");
    fflush(stdout);

    while (running) {
        struct pollfd poll_fd = {.fd = simulator.fd, .events = POLLIN};
        int timeout_ms = -1;
        int poll_result;

        if (simulator.measuring) {
            uint64_t now = monotonic_ms();
            timeout_ms = now >= simulator.next_measurement_ms ? 0 :
                         (int)(simulator.next_measurement_ms - now);
            if (timeout_ms > 100)
                timeout_ms = 100;
        }
        poll_result = poll(&poll_fd, 1, timeout_ms);
        if (poll_result < 0) {
            if (errno == EINTR)
                continue;
            perror("poll /dev/uhid");
            break;
        }
        if (poll_result > 0 && (poll_fd.revents & POLLIN)) {
            ssize_t bytes_read;
            memset(&event, 0, sizeof(event));
            do {
                bytes_read = read(simulator.fd, &event, sizeof(event));
            } while (bytes_read < 0 && errno == EINTR);
            if (bytes_read < 0) {
                perror("read /dev/uhid");
                break;
            }
            if (bytes_read == 0)
                break;
            switch (event.type) {
            case UHID_START:
                puts("UHID_START");
                break;
            case UHID_OPEN:
                puts("UHID_OPEN");
                break;
            case UHID_CLOSE:
                puts("UHID_CLOSE");
                break;
            case UHID_STOP:
                puts("UHID_STOP");
                break;
            case UHID_OUTPUT:
                if (process_command(&simulator, event.u.output.data,
                                    event.u.output.size) != 0)
                    running = 0;
                break;
            case UHID_SET_REPORT:
                if (reply_set_report(&simulator, event.u.set_report.id) != 0 ||
                    process_command(&simulator, event.u.set_report.data,
                                    event.u.set_report.size) != 0)
                    running = 0;
                break;
            case UHID_GET_REPORT:
                if (reply_get_report(&simulator, event.u.get_report.id) != 0)
                    running = 0;
                break;
            default:
                printf("UHID event type=%u\n", event.type);
                break;
            }
            fflush(stdout);
        }
        if (simulator.measuring && monotonic_ms() >= simulator.next_measurement_ms &&
            send_measurement(&simulator) != 0)
            running = 0;
    }

    memset(&event, 0, sizeof(event));
    event.type = UHID_DESTROY;
    if (write_uhid_event(simulator.fd, &event) != 0)
        ret = 1;
    else {
        puts("Removed virtual Go!Link HID.");
        ret = 0;
    }
    close(simulator.fd);
    return ret;
}

static int self_test(void)
{
    struct simulator simulator = {
        .fd = -1,
        .period_ms = GOIO_DEFAULT_PERIOD_MS,
        .analog_channel = 1,
    };
    struct packet_set set;
    uint8_t command[GOIO_PACKET_SIZE] = {0};
    uint8_t flash[20] = {0};
    uint8_t measurement[GOIO_PACKET_SIZE];

    command[0] = 0x1a;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || set.packets[0][0] != 0x9a || set.packets[0][2] != 0)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x10;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || set.packets[0][0] != 0x5e || set.packets[0][1] != 0x10)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x17;
    command[2] = sizeof(flash);
    build_command_responses(command, &simulator, &set);
    if (set.count != 3 || set.packets[0][0] != 0x4f ||
        set.packets[1][0] != 0x47 || set.packets[2][0] != 0x57 ||
        set.packets[0][1] != 0x17)
        return 1;
    memcpy(flash, set.packets[0] + 2, 6);
    memcpy(flash + 6, set.packets[1] + 1, 7);
    memcpy(flash + 13, set.packets[2] + 1, 7);
    if (flash[1] != 0x9c || flash[8] != 0x3f || flash[9] != 0x80 ||
        flash[12] != 0x3f || flash[13] != 0x80)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x28;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || set.packets[0][0] != 0x5f ||
        set.packets[0][1] != 0x28 || set.packets[0][2] != 2)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x29;
    command[1] = 2;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || simulator.analog_channel != 2 ||
        set.packets[0][0] != 0x5a || set.packets[0][1] != 0x29)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x2a;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || set.packets[0][0] != 0x5a ||
        set.packets[0][1] != 0x2a || set.packets[0][2] != 2)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x1b;
    command[1] = 0xfa;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || simulator.period_ms != 250 || set.packets[0][0] != 0x5a)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x1c;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || set.packets[0][0] != 0x5d ||
        read_le32(set.packets[0] + 2) != 250)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x1e;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || set.packets[0][0] != 0x5b || set.packets[0][1] != 0x1e)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x20;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || set.packets[0][0] != 0x6f || set.packets[0][1] != 0x20)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x2b;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || set.packets[0][0] != 0x5a || set.packets[0][1] != 0x2b)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x18;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || !set.starts_measurements || set.packets[0][1] != 0x18)
        return 1;

    memset(command, 0, sizeof(command));
    command[0] = 0x19;
    build_command_responses(command, &simulator, &set);
    if (set.count != 1 || !set.stops_measurements || set.packets[0][1] != 0x19)
        return 1;

    build_measurement_packet(0, 7, measurement);
    if (measurement[0] != 1 || measurement[1] != 7 ||
        measurement[2] != 0x00 || measurement[3] != 0x20)
        return 1;

    puts("Go!Link protocol self-test passed (status, init, flash, sensor/channel, "
         "period, LED, serial, start/stop, measurement format).");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--self-test") == 0)
        return self_test();
    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--self-test]\n", argv[0]);
        return 2;
    }
    return run_device();
}
