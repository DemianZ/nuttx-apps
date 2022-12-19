/****************************************************************************
 * apps/examples/hello/hello_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <sys/ioctl.h>
#include <nuttx/config.h>
#include <nuttx/spi/spi.h>
#include <stdio.h>
#include <fcntl.h>

static const int SPI_PORT_TEST = 1; 

// TODO: Implement write operation for SPI driver
static int spi_char_driver_write(uint8_t * data, size_t len)    
{
    int fd = open("/dev/spi1", O_RDWR);
    if(fd < 0) {
      printf("Error opening SPI\n");
      return -1;
    }
    printf("SPI Opened\n");
    int bytes_written = write(fd, data, len); // write operation is not implemented
    if(bytes_written != len) {
        printf("Error sending\n");
        return -2;
    }
    printf("SPI Data sent\n");
    close(fd);
    printf("SPI Closed\n");
}

static int spi_dummy_byte_write(struct spi_dev_s * spi, uint8_t byte)
{
  if(spi == NULL){
    return -1;
  }
  
  SPI_SELECT(spi, 0, true);
  SPI_SEND(spi, byte);
  SPI_SELECT(spi, 0, false);

  return 0;
}

static int spi_bus_init(struct spi_dev_s ** spi, uint8_t spi_port)
{
  *spi = stm32_spibus_initialize(spi_port);
  if (spi == NULL) {
    return -ENODEV;
  }
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * spi_test_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{

  struct spi_dev_s * test_spi;

  int ret = spi_bus_init(&test_spi, SPI_PORT_TEST);
  if(ret) {
    printf("ERROR: Failed to initialize SPI port %d\n", SPI_PORT_TEST);
    return ret;
  }
  printf("SPI%d initialized\n", SPI_PORT_TEST);

  uint16_t counter = 0;
  while(counter++ < 100) {
    if(spi_dummy_byte_write(test_spi, counter)) {
      printf("Error sending SPI data\n");
      return -1;  
    };
  }

  printf("SPI Data sent\n");

  return 0;
}
