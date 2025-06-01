#pragma once

enum irq_source_t {
    UNKNOWN = 0,
    MEGA2_IRQ,
    VGC_IRQ,
    DOC_IRQ,
    ADB_IRQ
};

void raiseInterrupt(irq_source_t);
void lowerInterrupt(irq_source_t);