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

#include <nuttx/config.h>
#include <nuttx/spi/spi.h>
#include <stdio.h>


static const int TEST_SPI_PORT = 1; 

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * spi_test_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct spi_dev_s *spi;

  spi = stm32_spibus_initialize(TEST_SPI_PORT);
  if (spi == NULL)
  {
    printf("ERROR: Failed to initialize SPI port %d\n", TEST_SPI_PORT);
    return -ENODEV;
  }
  printf("SPI%d initialized\n", TEST_SPI_PORT);

  SPI_SELECT(spi, 0, true);
  SPI_SEND(spi, 0xAA);
  SPI_SEND(spi, 0x00);
  SPI_SEND(spi, 0xAA);
  SPI_SELECT(spi, 0, false);

  printf("SPI Data sent\n");


  return 0;
}
