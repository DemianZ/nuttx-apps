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
#include <fcntl.h>
#include <nuttx/config.h>
#include <nuttx/spi/spi.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>


/****************************************************************************
 * Private Data
 ****************************************************************************/
static const int SPI_PORT_TEST = 1; 
static bool g_spi_task1_daemon_started = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spi_char_driver_write
 ****************************************************************************/

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
    return 0;
}

/****************************************************************************
 * Name: spi_dummy_byte_write
 ****************************************************************************/

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

/****************************************************************************
 * Name: spi_bus_init
 ****************************************************************************/

static int spi_bus_init(struct spi_dev_s ** spi, uint8_t spi_port)
{
  *spi = (struct spi_dev_s *)stm32_spibus_initialize(spi_port);
  if (spi == NULL) {
    return -ENODEV;
  }
  return 0;
}


/****************************************************************************
 * Name: sigterm_action
 ****************************************************************************/

static void sigterm_action(int signo, siginfo_t *siginfo, void *arg)
{
  if (signo == SIGTERM)
  {
    printf("SIGTERM received\n");
    g_spi_task1_daemon_started = false;
    printf("spi_test_daemon: Terminated.\n");
  }
  else {
    printf("\nsigterm_action: Received signo=%d siginfo=%p arg=%p\n", signo, siginfo, arg);
  }
}

static int error_handler() 
{
  g_spi_task1_daemon_started = false;
  printf("spi_test_daemon: Terminating\n");
  return EXIT_FAILURE;
}

static int spi_test_task1(int argc, char *argv[]) 
{
  pid_t mypid;
  struct sigaction act;

  /* SIGTERM handler */

  memset(&act, 0, sizeof(struct sigaction));
  act.sa_sigaction = sigterm_action;
  act.sa_flags     = SA_SIGINFO;

  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, SIGTERM);

  int ret = sigaction(SIGTERM, &act, NULL);
  if (ret != 0) {
    fprintf(stderr, "Failed to install SIGTERM handler, errno=%d\n", errno);
    exit(error_handler());
  }

  mypid = getpid();
  g_spi_task1_daemon_started = true;
  printf("\nspi_task1_daemon (pid# %d): Running\n", mypid);

  struct spi_dev_s * test_spi;
  ret = spi_bus_init(&test_spi, SPI_PORT_TEST);
  if(ret) {
    fprintf(stderr, "Failed to initialize SPI port %d\n", SPI_PORT_TEST);
    exit(error_handler());
  }
  printf("SPI%d Initialized\n", SPI_PORT_TEST);

  uint16_t counter = 0;
  while (g_spi_task1_daemon_started == true) {
    if(spi_dummy_byte_write(test_spi, counter)) 
    {
      fprintf(stderr, "Error sending SPI data\n");
      exit(error_handler());
    };
    printf("SPI Data sent: %d\n", counter);
    counter++;
    usleep(1000 * 1000L);
  }

  exit(EXIT_SUCCESS);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * spi_test_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret = task_create(
    "spi_task1", 
    CONFIG_EXAMPLES_SPI_TEST_PRIORITY,
    CONFIG_EXAMPLES_SPI_TEST_STACKSIZE, 
    spi_test_task1,
    NULL);
    
  if (ret < 0) {
    int errcode = errno;
    fprintf(stderr, "Failed to start spi_task1: %d\n", errcode);
    return EXIT_FAILURE;
  }

  printf("spi_test_main: spi tasks started\n");

  return EXIT_SUCCESS;
}
