#pragma once

namespace Mega2 {

void start(void);
void stop(void);
void reset(void);
void tick(const unsigned int);
void microtick(const unsigned int);
void updateMemoryMaps();
void buildLanguageCard(unsigned int bank, unsigned int src_bank);

} // namespace Mega2