#define LEP_ASSERT_CF(A,C,F,...) ASSERT_CF(A,C,F,__VA_ARGS__)
//#define LEP_NOTE(C,F,...) note(C,F,__VA_ARGS__)

#include "debug.h"
#include "lep.h"
#include "crc.h"
#include "common.h"


//printf
#include <stdio.h>

//timerfd_create
#include <sys/timerfd.h>

//read
#include <unistd.h>

//strerror
#include <string.h>

//errrno
#include <errno.h>


#include <sys/epoll.h>




int app_error_frames = 0;
	
void app_read_packet (int dev)
{
	struct Lep_Packet pack [LEP2_HEIGHT];
	lep_spi_receive (dev, (uint8_t *) pack, sizeof (pack));
	
	if (!lep_check (pack)) 
	//if ((pack [0].reserved & 0x0F) == 0x0F)
	{
		usleep (app_error_frames); 
		app_error_frames ++; 
		printf ("e : %i\n", app_error_frames);
		return;
	}
	app_error_frames = 0;
	
	if (lep_check (pack + 20) && (pack [20].number != 20)) 
	{lep_spi_receive (dev, (uint8_t *) pack, LEP_PACKET_SIZE); return;}
	
	for (size_t i = 0; i < LEP2_HEIGHT; i = i + 1)
	{
		if (lep_check (pack + i)) {printf ("%s", TCOL (TCOL_BOLD, TCOL_GREEN, TCOL_DEFAULT));}
		else {printf ("%s", TCOL (TCOL_BOLD, TCOL_RED, TCOL_DEFAULT));}
		printf ("%02x ", pack [i].number);
		printf ("%s", TCOL_RESET);
	}
	printf ("\n");
}



int main (int argc, char * argv [])
{
	int C = 1;
	while (C != -1)
	{
		C = getopt (argc, argv, "");
		switch (C)
		{	
			default:
			break;
		}
	}
	
	
	int efd;
	int tfd;
	int pinfd;
	int counter = 0;
	int dev = lep_spi_open (LEP_SPI_DEV_RPI3);
	
	struct epoll_event events [10];
	
	tfd = timerfd_create (CLOCK_MONOTONIC, 0);
	ASSERT_F (tfd > 0, "%s", "");
	
	{
		struct itimerspec ts;
		ts.it_interval.tv_sec = 1;
		ts.it_interval.tv_nsec = 0;
		ts.it_value.tv_sec = 1;
		ts.it_value.tv_nsec = 0;
		int r = timerfd_settime (tfd, 0, &ts, NULL);
		ASSERT_F (r == 0, "%s", "");
	}
	
	pinfd = lep_isr_init (17);
	printf ("pinfd %i\n", pinfd);
	efd = epoll_create1 (0);
	ASSERT_F (efd > 0, "%s", "");
	
	//app_epoll_add (efd, EPOLLIN | EPOLLET, tfd);
	app_epoll_add (efd, EPOLLIN | EPOLLPRI | EPOLLET, pinfd);
	
	
	while (1)
	{
		//printf ("waiting for events...\n");
		int n = epoll_wait (efd, events, 10, -1);
		ASSERT_F (n > 0, "%s", "");
		//printf ("new events %i!\n", n);
		for (int i = 0; i < n; i = i + 1)
		{
			if (events [i].data.fd == pinfd)
			{
				app_read_packet (dev);
				char c;
				lseek (pinfd, 0, SEEK_SET);
				int res = read (pinfd, &c, 1);
				ASSERT_F (res == 1, "%s", "");
				counter ++;
			}
			
			else if (events [i].data.fd == tfd)
			{
				uint64_t m;
				int res = read (tfd, &m, sizeof (m));
				ASSERT_F (res == sizeof (m), "%s", "");
				printf ("tfd %i\n", counter);
				counter = 0;
			}
		}
	}
	
	
	return 0;
}

