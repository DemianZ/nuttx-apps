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
 #include <mqueue.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>


/****************************************************************************
 * Module Defines
 ****************************************************************************/
#define PRINT_MATRIX  (0)

/****************************************************************************
 * Private Data
 ****************************************************************************/
struct spi_dev_s * test_spi;
static const int SPI_PORT_TEST = 1; 
static bool g_spi_task1_daemon_started = 0;
static bool g_spi_task2_daemon_started = 0;
static bool g_spi_task_sender_daemon_started = 0;
static const size_t MATRIX_DIM = 3;

static struct mq_attr spi_mq_attr = {
  .mq_curmsgs = 0,
  .mq_flags = 0,
  .mq_maxmsg = 10,
  .mq_msgsize = 10
};

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
 * Name: spi_dummy_write
 ****************************************************************************/

static int spi_dummy_write(struct spi_dev_s * spi, uint8_t * data, size_t len)
{
  if(spi == NULL){
    return -1;
  }
  
  SPI_SELECT(spi, 0, true);
  for(size_t i = 0; i < len; i++) {
    SPI_SEND(spi, data[i]);
  }
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
 * Name: error_handler
 ****************************************************************************/

static int error_handler() 
{
  g_spi_task1_daemon_started = false;
  g_spi_task2_daemon_started = false;
  g_spi_task_sender_daemon_started = false;
  
  printf("spi_test_daemon: Terminating\n");
  return EXIT_FAILURE;
}


/****************************************************************************
 * Name: generate_rand_matrix
 ****************************************************************************/

static void generate_rand_matrix(int matrix[MATRIX_DIM][MATRIX_DIM])
{
  for(size_t i=0; i < MATRIX_DIM; i++){
    for(size_t j=0; j < MATRIX_DIM; j++) {
      matrix[i][j] = rand();
    }
  }
}

/****************************************************************************
 * Name: print_matrix
 ****************************************************************************/

static void print_matrix(int matrix[MATRIX_DIM][MATRIX_DIM])
{
  for(size_t i = 0; i < MATRIX_DIM; i++) {
    for(size_t j = 0; j < MATRIX_DIM; j++) {
      printf("%d ", matrix[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}

/****************************************************************************
 * Name: multiply_matrix
 ****************************************************************************/

static void multiply_matrix(
  int matrix1[MATRIX_DIM][MATRIX_DIM],
  int matrix2[MATRIX_DIM][MATRIX_DIM],  
  int res_matrix[MATRIX_DIM][MATRIX_DIM]) {
  // Initializing elements of res_matrix to 0
	for(size_t i = 0; i < MATRIX_DIM; ++i) {
		for(size_t j = 0; j < MATRIX_DIM; ++j) {
			res_matrix[i][j] = 0;
		}
	}

  // Multiply matrix elements
  // TODO: handle MAX_INT overflow
  for(size_t i = 0; i < MATRIX_DIM; i++) {
		for(size_t j = 0; j < MATRIX_DIM; j++) {
			for(size_t k = 0; k < MATRIX_DIM; k++) {
				res_matrix[i][j] += matrix1[i][k] * matrix2[k][j]; 
			}
		}
	}
}

/****************************************************************************
 * Name: spi_send_matrix
 ****************************************************************************/

static int spi_send_matrix(int matrix[MATRIX_DIM][MATRIX_DIM])
{
  const uint8_t buf_len = MATRIX_DIM*MATRIX_DIM;
  uint8_t buf[buf_len];
  for(size_t i = 0; i < MATRIX_DIM; i++) {
		for(size_t j = 0; j < MATRIX_DIM; j++) {
			buf[i*3 + j] = matrix[i][j];
		}
	}
  return spi_dummy_write(test_spi, buf, buf_len);
}

void spi_send_from_queue(uint8_t * data, size_t len)
{

}


/****************************************************************************
 * Name: spi_test_task1
 ****************************************************************************/

static int spi_test_task1(int argc, char *argv[]) 
{

  pid_t mypid;
  mypid = getpid();
  g_spi_task1_daemon_started = true;
  printf("spi_task1_daemon (pid# %d): Running\n", mypid);

  mqd_t mq_spi;
  mq_spi = mq_open("mq_spi", O_RDWR|O_CREAT, 0666, &spi_mq_attr);
  if (mq_spi == -1) {
    fprintf(stderr, "spi_send queue not opened: %d\n", errno);
    exit(error_handler());
  }

  uint16_t counter = 0;
  while (g_spi_task1_daemon_started == true) {

    int status = mq_send(mq_spi, &counter, sizeof(counter), 100);
    if (status < 0) {
      printf("ERROR mq_send failure=%d\n", status);
    }
    printf("SPI counter sent to queue: %d\n", counter);
    counter++;
    usleep(1000 * 1000L);
  }

  mq_close(mq_spi);
  exit(EXIT_SUCCESS);
}

/****************************************************************************
 * Name: spi_test_task2
 ****************************************************************************/

static int spi_test_task2(int argc, char *argv[]) 
{
  pid_t mypid;  
  mypid = getpid();
  g_spi_task2_daemon_started = true;
  printf("spi_task2_daemon (pid# %d): Running\n", mypid);

  int matrix1[MATRIX_DIM][MATRIX_DIM];
  int matrix2[MATRIX_DIM][MATRIX_DIM];
  int mult_matrix[MATRIX_DIM][MATRIX_DIM];

  mqd_t mq_spi;
  mq_spi = mq_open("mq_spi", O_RDWR|O_CREAT, 0666, &spi_mq_attr);
  if (mq_spi == -1) {
    fprintf(stderr, "spi_send queue not opened: %d\n", errno);
    exit(error_handler());
  }

  while (g_spi_task2_daemon_started == true) {

    generate_rand_matrix(matrix1);
    generate_rand_matrix(matrix2);
    multiply_matrix(matrix1, matrix2, mult_matrix);

#if(PRINT_MATRIX)
    print_matrix(matrix1);
    print_matrix(matrix2);
    print_matrix(mult_matrix);
#endif

    const uint8_t buf_len = MATRIX_DIM*MATRIX_DIM;
    uint8_t buf[buf_len];
    for(size_t i = 0; i < MATRIX_DIM; i++) {
      for(size_t j = 0; j < MATRIX_DIM; j++) {
        buf[i*3 + j] = mult_matrix[i][j];
      }
    }

    int status = mq_send(mq_spi, buf, buf_len, 100);
    if (status < 0) {
      printf("ERROR mq_send failure=%d\n", status);
    }
    
    printf("SPI matrix sent to queue\n");
    usleep(1000 * 1000L);
  }

  mq_close(mq_spi);
  exit(EXIT_SUCCESS);
}

/****************************************************************************
 * Name: spi_sender_task
 ****************************************************************************/

static int spi_sender_task(int argc, char *argv[]) 
{
  pid_t mypid;  
  mypid = getpid();
  g_spi_task_sender_daemon_started = true;
  printf("spi_sender_task_daemon (pid# %d): Running\n", mypid);

  mqd_t mq_spi;
  mq_spi = mq_open("mq_spi", O_RDWR|O_CREAT, 0666, &spi_mq_attr);
  if (mq_spi == -1) {
    fprintf(stderr, "spi_send queue not opened: %d\n", errno);
    exit(error_handler());
  }

  uint8_t queue_rx_buf[20];
  int prio=100;
  while (g_spi_task_sender_daemon_started == true) {
    size_t nbytes = mq_receive(mq_spi, queue_rx_buf, 20, &prio);
    if(nbytes > 0) {
      if(spi_dummy_write(test_spi, queue_rx_buf, nbytes)) {
        fprintf(stderr, "SPI write failed\n");
      } else {
        printf("SPI sent %i bytes\n", nbytes);
      }
    }
  }

  mq_close(mq_spi);
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
  int ret = spi_bus_init(&test_spi, SPI_PORT_TEST);
  if(ret) {
    fprintf(stderr, "Failed to initialize SPI port %d\n", SPI_PORT_TEST);
    exit(error_handler());
  }
  printf("SPI%d Initialized\n", SPI_PORT_TEST);

  
  ret = task_create(
    "spi_task1", 
    CONFIG_EXAMPLES_SPI_TEST_PRIORITY,
    CONFIG_EXAMPLES_SPI_TEST_STACKSIZE, 
    spi_test_task1,
    NULL);
  if (ret < 0) {
    int errcode = errno;
    fprintf(stderr, "Failed to start spi_task2: %d\n", errcode);
    return EXIT_FAILURE;
  }

  ret = task_create(
    "spi_task2", 
    CONFIG_EXAMPLES_SPI_TEST_PRIORITY,
    CONFIG_EXAMPLES_SPI_TEST_STACKSIZE, 
    spi_test_task2,
    NULL);
  if (ret < 0) {
    int errcode = errno;
    fprintf(stderr, "Failed to start spi_task2: %d\n", errcode);
    return EXIT_FAILURE;
  }

  ret = task_create(
    "spi_sender_task", 
    CONFIG_EXAMPLES_SPI_TEST_PRIORITY,
    CONFIG_EXAMPLES_SPI_TEST_STACKSIZE, 
    spi_sender_task,
    NULL);
  if (ret < 0) {
    int errcode = errno;
    fprintf(stderr, "Failed to start spi_task_sender: %d\n", errcode);
    return EXIT_FAILURE;
  }

  printf("spi_test_main: spi tasks started\n");

  return EXIT_SUCCESS;
}
