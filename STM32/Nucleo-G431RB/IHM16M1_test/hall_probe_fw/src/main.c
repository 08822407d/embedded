#include "stm32g431xx.h"

#include <stdint.h>

#define H1_PIN 6u
#define H2_PIN 7u
#define H3_PIN 8u
#define HALL_MASK ((1u << H1_PIN) | (1u << H2_PIN) | (1u << H3_PIN))

#define UART_BAUD 115200u
#define SYSCLK_HZ 16000000u

uint32_t SystemCoreClock = SYSCLK_HZ;

static uint32_t tick_ms;

void SystemInit(void)
{
    SystemCoreClock = SYSCLK_HZ;
}

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = SYSCLK_HZ;
}

void __libc_init_array(void)
{
}

static void delay_ms(uint32_t ms)
{
    while (ms-- != 0u) {
        for (volatile uint32_t i = 0; i < 3900u; ++i) {
            __NOP();
        }
        ++tick_ms;
    }
}

static void clock_init(void)
{
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0u) {
    }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
    }

    SystemCoreClock = SYSCLK_HZ;
}

static void gpio_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
    (void)RCC->AHB2ENR;

    GPIOA->MODER &= ~((3u << (2u * 2u)) | (3u << (3u * 2u)));
    GPIOA->MODER |= (2u << (2u * 2u)) | (2u << (3u * 2u));
    GPIOA->AFR[0] &= ~((0xFu << (4u * 2u)) | (0xFu << (4u * 3u)));
    GPIOA->AFR[0] |= (7u << (4u * 2u)) | (7u << (4u * 3u));
    GPIOA->OSPEEDR |= (2u << (2u * 2u)) | (2u << (3u * 2u));
    GPIOA->PUPDR &= ~((3u << (2u * 2u)) | (3u << (3u * 2u)));
    GPIOA->PUPDR |= (1u << (3u * 2u));

    GPIOC->MODER &= ~((3u << (H1_PIN * 2u)) |
                      (3u << (H2_PIN * 2u)) |
                      (3u << (H3_PIN * 2u)));
    GPIOC->PUPDR &= ~((3u << (H1_PIN * 2u)) |
                      (3u << (H2_PIN * 2u)) |
                      (3u << (H3_PIN * 2u)));
    GPIOC->PUPDR |= (1u << (H1_PIN * 2u)) |
                    (1u << (H2_PIN * 2u)) |
                    (1u << (H3_PIN * 2u));
}

static void uart_init(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    (void)RCC->APB1ENR1;

    USART2->CR1 = 0u;
    USART2->CR2 = 0u;
    USART2->CR3 = 0u;
    USART2->BRR = (SYSCLK_HZ + (UART_BAUD / 2u)) / UART_BAUD;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void uart_putc(char c)
{
    while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0u) {
    }
    USART2->TDR = (uint8_t)c;
}

static void uart_puts(const char *text)
{
    while (*text != '\0') {
        if (*text == '\n') {
            uart_putc('\r');
        }
        uart_putc(*text++);
    }
}

static void uart_put_u32(uint32_t value)
{
    char buf[10];
    uint32_t n = 0u;

    if (value == 0u) {
        uart_putc('0');
        return;
    }

    while (value != 0u && n < sizeof(buf)) {
        buf[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (n != 0u) {
        uart_putc(buf[--n]);
    }
}

static int uart_getc_nonblock(char *out)
{
    if ((USART2->ISR & USART_ISR_RXNE_RXFNE) == 0u) {
        return 0;
    }

    *out = (char)(USART2->RDR & 0xFFu);
    return 1;
}

static uint8_t hall_read_raw(void)
{
    uint32_t idr = GPIOC->IDR;
    uint8_t h1 = (uint8_t)((idr >> H1_PIN) & 1u);
    uint8_t h2 = (uint8_t)((idr >> H2_PIN) & 1u);
    uint8_t h3 = (uint8_t)((idr >> H3_PIN) & 1u);

    return (uint8_t)(h1 | (h2 << 1u) | (h3 << 2u));
}

static int8_t hall_sector(uint8_t state)
{
    static const int8_t sector_lut[8] = {
        -1, 0, 4, 5, 2, 1, 3, -1
    };

    return sector_lut[state & 7u];
}

static void print_state(uint8_t state,
                        uint32_t transitions,
                        uint32_t h1_rises,
                        uint32_t invalid_states,
                        uint32_t skipped_transitions,
                        uint32_t last_delta_ms)
{
    int8_t sector = hall_sector(state);

    uart_puts("t_ms=");
    uart_put_u32(tick_ms);
    uart_puts(" hall=");
    uart_putc((state & 1u) ? '1' : '0');
    uart_putc((state & 2u) ? '1' : '0');
    uart_putc((state & 4u) ? '1' : '0');
    uart_puts(" sector=");
    if (sector < 0) {
        uart_puts("invalid");
    } else {
        uart_put_u32((uint32_t)sector);
        uart_puts(" elec_deg_nom=");
        uart_put_u32((uint32_t)sector * 60u);
    }
    uart_puts(" trans=");
    uart_put_u32(transitions);
    uart_puts(" h1_rise=");
    uart_put_u32(h1_rises);
    uart_puts(" invalid=");
    uart_put_u32(invalid_states);
    uart_puts(" skipped=");
    uart_put_u32(skipped_transitions);
    uart_puts(" dt_ms=");
    uart_put_u32(last_delta_ms);
    uart_puts("\n");
}

static uint8_t wait_stable_hall(uint8_t previous)
{
    uint8_t candidate = previous;
    uint8_t same_count = 0u;

    for (uint8_t i = 0u; i < 8u; ++i) {
        uint8_t raw = hall_read_raw();
        if (raw == candidate) {
            if (same_count < 3u) {
                ++same_count;
            }
        } else {
            candidate = raw;
            same_count = 1u;
        }

        delay_ms(1u);
    }

    return candidate;
}

static void print_banner(void)
{
    uart_puts("\nHALL_PROBE_FW STM32G431RB / X-NUCLEO-IHM16M1\n");
    uart_puts("Safe mode: reads PC6/PC7/PC8 only; no PWM, no TIM1, no bridge enable, no motor drive.\n");
    uart_puts("USART2 VCP 115200 8N1. Send 'r' to reset counters.\n");
    uart_puts("Ask user to rotate the motor shaft by hand only with motor bus disconnected.\n\n");
}

int main(void)
{
    uint8_t state;
    uint8_t prev_state;
    uint32_t transitions = 0u;
    uint32_t h1_rises = 0u;
    uint32_t invalid_states = 0u;
    uint32_t skipped_transitions = 0u;
    uint32_t last_change_ms = 0u;
    uint32_t last_print_ms = 0u;

    clock_init();
    gpio_init();
    uart_init();
    delay_ms(50u);
    print_banner();

    state = wait_stable_hall(hall_read_raw());
    prev_state = state;
    if (hall_sector(state) < 0) {
        ++invalid_states;
    }
    print_state(state, transitions, h1_rises, invalid_states, skipped_transitions, 0u);

    for (;;) {
        char rx;
        uint8_t next_state;

        if (uart_getc_nonblock(&rx) != 0) {
            if (rx == 'r' || rx == 'R') {
                transitions = 0u;
                h1_rises = 0u;
                invalid_states = 0u;
                skipped_transitions = 0u;
                last_change_ms = tick_ms;
                prev_state = wait_stable_hall(hall_read_raw());
                uart_puts("Counters reset.\n");
                print_state(prev_state, transitions, h1_rises, invalid_states, skipped_transitions, 0u);
            }
        }

        next_state = wait_stable_hall(prev_state);
        if (next_state != prev_state) {
            int8_t prev_sector = hall_sector(prev_state);
            int8_t next_sector = hall_sector(next_state);
            uint8_t changed_bits = (uint8_t)(prev_state ^ next_state);
            uint32_t now_ms = tick_ms;
            uint32_t delta_ms = now_ms - last_change_ms;

            if (next_sector < 0) {
                ++invalid_states;
            } else if (prev_sector >= 0 &&
                       (changed_bits == 1u || changed_bits == 2u || changed_bits == 4u)) {
                ++transitions;
                if (((prev_state & 1u) == 0u) && ((next_state & 1u) != 0u)) {
                    ++h1_rises;
                }
                last_change_ms = now_ms;
            } else {
                ++skipped_transitions;
            }

            prev_state = next_state;
            print_state(prev_state, transitions, h1_rises, invalid_states, skipped_transitions, delta_ms);
            last_print_ms = tick_ms;
        } else if ((tick_ms - last_print_ms) >= 1000u) {
            print_state(prev_state,
                        transitions,
                        h1_rises,
                        invalid_states,
                        skipped_transitions,
                        tick_ms - last_change_ms);
            last_print_ms = tick_ms;
        }
    }
}
