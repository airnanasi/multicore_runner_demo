#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/sync.h>
#include <pico/util/queue.h>

#define SHARED_DATA_SIZE 16

// 共有メモリ（配列）
volatile int shared_data[SHARED_DATA_SIZE];

// セマフォ
semaphore_t sem;

// キュー（Core1 → Core0 の通知用）
queue_t core1_to_core0_queue;

// Core1の処理（送信側）
void core1_main() {
    int data_to_send = 0;

    while (true) {
        // セマフォを取得して共有メモリにデータを書き込む
        sem_acquire_blocking(&sem);

        printf("Core 1: Writing data to shared memory...\n");
        for (int i = 0; i < SHARED_DATA_SIZE; i++) {
            shared_data[i] = data_to_send + i;
        }
        data_to_send += SHARED_DATA_SIZE;

        // セマフォを解放
        sem_release(&sem);

        // Core0に通知を送る
        uint32_t notification = 1; // 任意の値（ここでは1）
        queue_add_blocking(&core1_to_core0_queue, &notification);

        printf("Core 1: Data written and notification sent to Core 0.\n");

        // 少し待機
        sleep_ms(2000);
    }
}

int main() {
    stdio_init_all();
    printf("Starting Core 1...\n");

    // セマフォの初期化（初期値1、最大値1のバイナリセマフォ）
    sem_init(&sem, 1, 1);

    // キューの初期化（Core1からCore0への通知用）
    queue_init(&core1_to_core0_queue, sizeof(uint32_t), 1);

    // Core1を起動
    multicore_launch_core1(core1_main);

    while (true) {
        // Core1からの通知を待機
        uint32_t notification;
        queue_remove_blocking(&core1_to_core0_queue, &notification);

        // セマフォを取得して共有メモリからデータを読み取る
        sem_acquire_blocking(&sem);

        printf("Core 0: Reading data from shared memory...\n");
        for (int i = 0; i < SHARED_DATA_SIZE; i++) {
            printf("Core 0: shared_data[%d] = %d\n", i, shared_data[i]);
        }

        // セマフォを解放
        sem_release(&sem);

        // 少し待機
        sleep_ms(10);
    }

    return 0;
}
