#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// Hardware configuration
RF24 radio(9,10);                          // Set up nRF24L01 radio on SPI bus plus pins 7 & 8

// Pin configurable address
short addr_idx = 0;
const short addr_0 = 5;
const short addr_1 = 6;
const short addr_2 = 7;

// First address is master and up to 5 slave devices can be controlled
byte address[][6] = { "0mstr",
    "1slav",
    "2slav",
    "3slav",
    "4slav",
    "5slav" };

// Are we acting as master or slave?
typedef enum { master = 1, slave } Role;
const char* role_friendly_name[] = { "invalid", "master", "slave"};
Role role;

// Are we waiting to hear back from the slaves?
short num_slaves = 1;

//
short ack_addr;
volatile bool waiting;

static uint32_t count = 0;

// ISR forward decl.
void isr_slave(void);
void isr_master(void);

void setup() {

    // TODO: remove
    Serial.begin(115200);

    // Hardware configurable address, 0-5 according to address definition
    pinMode(addr_0, INPUT);
    pinMode(addr_1, INPUT);
    pinMode(addr_2, INPUT);

    // Set address here, for testing purposes
    digitalWrite(addr_0, LOW);
    digitalWrite(addr_1, LOW);
    digitalWrite(addr_2, LOW);

    // Allow digital write to settle
    delay(20);

    // Who am I?
    pinMode(13, OUTPUT);
    addr_idx = 10;
    while (addr_idx > 5) {

        addr_idx =
            digitalRead(addr_0) << 0 |
            digitalRead(addr_1) << 1 |
            digitalRead(addr_2) << 2;

        // LED on for invalid address
        digitalWrite(13, HIGH);
    }

    Serial.print("Device address set to: ");
    Serial.println(addr_idx);

    digitalWrite(13, LOW);

    // Decide role
    if (addr_idx == 0)
        role = master;
    else
        role = slave;

    Serial.print(F("ROLE: "));
    Serial.println(role_friendly_name[role]);

    // Setup and configure radio
    radio.begin();
    radio.enableAckPayload();
    radio.enableDynamicPayloads();

    // Open pipes
    if (role == master) {

        radio.openWritingPipe(address[0]);
        radio.openReadingPipe(1,address[1]);
        radio.openReadingPipe(2,address[2]);
        radio.openReadingPipe(3,address[3]);
        radio.openReadingPipe(4,address[4]);
        radio.openReadingPipe(5,address[5]);

        // TODO: Search for slaves in range and popule a synchronization list

    } else {

        radio.openWritingPipe(address[addr_idx]);
        radio.openReadingPipe(1, address[0]);

        //radio.writeAckPayload(1, &addr_idx, sizeof(addr_idx) );
    }

    // Applies to both roles
    radio.setAutoAck(1);
    radio.setRetries(0,15);
    radio.startListening();
    radio.printDetails();
}

// Master send ticks, slaves receive and acknowledge entirely in isr
void loop() {

    // TODO: This should be driven an interupt coming from a master clock signal
    if (role == master)  {

        radio.stopListening();

        Serial.print(F("Sending count: "));
        Serial.println(count);

        if (!radio.write(&count, sizeof(count))) {

            Serial.println(F("failed."));

        } else if (!radio.available()) {

            Serial.println(F("Blank Payload Received."));

        } else {

            while (radio.available()) {
                short ack_idx;
                radio.read(&ack_idx, sizeof(ack_idx));
                Serial.print(F("Got Ack: "));
                Serial.println(ack_idx);
                count++;
            }
        }
    } else if (role == slave) {

        while(radio.available()) {
            uint32_t cnt;
            radio.read(&cnt, sizeof(cnt));
            Serial.print(F("Got count: "));
            Serial.println(cnt);
            radio.writeAckPayload(1, &addr_idx, sizeof(addr_idx));
        }
    }
}
