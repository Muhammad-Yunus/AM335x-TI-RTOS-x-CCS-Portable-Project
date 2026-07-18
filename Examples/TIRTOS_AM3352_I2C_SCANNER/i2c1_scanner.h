#ifndef I2C1_SCANNER_H_
#define I2C1_SCANNER_H_

void Board_initI2C1(void);
void SetupI2C1Master(void);
int  I2C1ProbeAddress(unsigned char addr);

#endif
