#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/sync.h>
#include <pico/util/queue.h>

#define SHARED_DATA_SIZE 16

// Variable for LED toggle
volatile bool led_state = false;

// Shared memory (array)
volatile int shared_data[SHARED_DATA_SIZE];

// Semaphore
semaphore_t sem;

// Queue (for notifications from Core1 to Core0)
queue_t core1_to_core0_queue;

// Core1 process (Sender side)
void core1_main() {
    int data_to_send = 0;

    while (true) {
        // Acquire semaphore and write data to shared memory
        sem_acquire_blocking(&sem);

        printf("Core 1: Writing data to shared memory...\n");
        for (int i = 0; i < SHARED_DATA_SIZE; i++) {
            shared_data[i] = data_to_send + i;
        }
        data_to_send += SHARED_DATA_SIZE;

        // Release semaphore
        sem_release(&sem);

        // Send notification to Core0
        uint32_t notification = 1; // Arbitrary value (1 in this case)
        queue_add_blocking(&core1_to_core0_queue, &notification);

        printf("Core 1: Data written and notification sent to Core 0.\n");

        // Short delay
        sleep_ms(2000);
    }
}

int main() {
    stdio_init_all();
    printf("Starting Core 1...\n");

    // GPIO 25 is the on-board LED
    bool led_state = false;
    const uint LED_PIN = 25;    // GPIO 25 is the on-board LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Initialize semaphore (binary semaphore with initial value 1, maximum value 1)
    sem_init(&sem, 1, 1);

    // Initialize queue (for notifications from Core1 to Core0)
    queue_init(&core1_to_core0_queue, sizeof(uint32_t), 1);

    // Launch Core1
    multicore_launch_core1(core1_main);

    while (true) {
        // Wait for notification from Core1
        uint32_t notification;
        queue_remove_blocking(&core1_to_core0_queue, &notification);

        gpio_put(LED_PIN, led_state);
        led_state = !led_state;

        // Acquire semaphore and read data from shared memory
        sem_acquire_blocking(&sem);

        printf("Core 0: Reading data from shared memory...\n");
        for (int i = 0; i < SHARED_DATA_SIZE; i++) {
            printf("Core 0: shared_data[%d] = %d\n", i, shared_data[i]);
        }

        // Release semaphore
        sem_release(&sem);

        // Short delay
        sleep_ms(10);
    }

    return 0;
}
