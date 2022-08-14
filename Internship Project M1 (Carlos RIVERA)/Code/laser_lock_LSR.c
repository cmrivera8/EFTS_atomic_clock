/* README
 *
 * You may want to pay attention to the variables tags with "LOOK AT ME"
 *  test
 */



// classic lib
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <fftw3.h>
#include <string.h>
#include <signal.h> // to catch Ctrl + C from keyboard and close socket

//udp client
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#define BUF_SIZE 60

// DAC driver
#include <axi_to_dac_conf.h>

#define ELEMENT_SIZE 2048
#define N ELEMENT_SIZE

// LOOK AT ME
#define DAC_ELEMENT_SIZE 16384
// This variable define the number of level of the DAC.
// This is valable for a 14-bits DAC (2**14 = 16384)
// You may want to change this if your DACs are differents


// PID
// LOOK AT ME
#define Ts 0.08
// Ts is the sample time of the wavelength-meter
// If you look in Wavelength.exe on your w10 PC (shame on you), you will see somewhere "sleep(TIME_CONSTANT)"
// Ts must be the same than TIME_CONSTANT
// DO NOT CHANGE Ts WITHOUT CHANGE TIME_CONSTANT
// TODO: Ts isn't use anymore



/************************************/
/************ LOOK AT ME ************/
/************************************/

// Uncomment this to use the PID implemented in this code.
// Otherwise the DACs output is only the error signal.
#define USE_REDPITAYA_PID

/************************************/
/************************************/
/************************************/





//udp
int sock;
struct sockaddr_in server_addr;
struct hostent *host;
char send_data[1024];
//char recv_data[BUF_SIZE];
double recv_data;
double recv_lambda;
int bytes_read;
socklen_t addr_len;
//eudp

struct timeval start,stop;
double elapsed_time;


void init_udp(char **argv);
void init_udp(char **argv){
	host= (struct hostent *) gethostbyname((char *)argv[1]);

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		perror("socket");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_addr.sin_zero),8);

	/*init dialog*/
	sprintf(send_data,"hello");
	sendto(sock, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
}

void close_udp(void);
void close_udp(void){
	/*end dialog*/
	sprintf(send_data,"Bye");
	sendto(sock, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
	close(sock);
}

//Try to close the socket
void sig_handler(int signum);
void sig_handler(int signum){
	printf("Closing socket, press Ctrl+C once again to quit the process\n");
	//signal(SIGINT,SIG_DFL);
	sprintf(send_data,"Bye");
	sendto(sock, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
	close(sock);
        printf("socket closed\n");
        raise(SIGTERM);
        
}


double my_pi(double order, double mesure, double Kp, double Ki, double Kd, double gain, double* err, double* err_1, double* err_2);
double my_pi(double order, double mesure, double Kp, double Ki, double Kd, double gain, double* err, double* err_1, double* err_2){
  
/*      LOOK AT ME 
	Bloc diagram of the PID controler implemented in Z-domain
                                                          
                                                                        ____  Pk
                                                      +--------------->|_Kp_|-----+
                                                      |                           |
                            +        ______    err    |   _________     ____  Ik  v   u    _____     mesure
	   order ----------->o----->|_gain_|----------+->|_z/(z-1)_|-->|_Ki_|---->o------>|_SYS_|-------------+
                             ^-                       |                           ^                           |
                             |                        |     _____       ____  Dk  |                           |
                             |                        +--->|_z-1_|---->|_Kd_|-----+                           |
                             |                                                                                |
                             |                                                                                |
                             |                                                                                |
                             +--------------------------------------------------------------------------------+

*/	


	double correction, Pk, Ik, Dk;

	// error calculation
	*err_2 = *err_1;
	*err_1 = *err;
	*err = gain*(order - mesure);

	// Pk calculation
	Pk = Kp * (*err - *err_1);
	
	// Ik calculation
	Ik = Ki * (*err);

	// Dk calculation
	Dk = Kd * (*err + *err_2 - 2*(*err_1))/2;

	// Correction calculation
	correction = Pk+Ik+Dk;
	
	// Return the result
	return correction;
}


int main( __attribute__ ((unused))int argc, char **argv){

	/************************/
	/* Controle the program */
	/************************/
	// TODO : find a better way of doing it on server side
	unsigned char stop_pid='0';	// if '1' then the code stops
	FILE* command_file;	// if you run "echo -ne '1' > stop_pid the code stops
	command_file = fopen("stop_pid", "w+");	// write the default file with a '0' inside
	if (!command_file) {
		printf("Not good, file inaccessible\n");
		return -1;
	}
	fputc('0', command_file);
	fclose(command_file);
	
	/*************************/
	/* Frequency measurement */
	/*************************/
	FILE* fd;
	char filename[256];				// data tab 
	char filenamelog[256];				// log tab
	sprintf(filename,"%s.dat",argv[3]);		// data file
	sprintf(filenamelog,"%s.log",argv[3]);		// log file

	fd=fopen(filename,"w");fclose(fd);		// create data file
	fd=fopen(filenamelog,"w");fclose(fd);		// create log file

	printf("test %lf \n",(double)CLOCKS_PER_SEC);	// ???
	gettimeofday(&start,NULL);			// get date
 	
        //signal(SIGINT,sig_handler);

	/*******/
	/* UDP */
	/*******/
	// LOOK AT ME
	char udppackage;				// To now what is the package received
	// 0 by default
	// 1 for output 1
	// 2 for output 2	

	int udpcount;					// To count the UDP package received
	init_udp(argv);					// Connect to the UDP server (ie the soft on windows)
	
	/*******/
	/* DAC */
	/*******/
	/* configuration
	 * 	chanA is set to 0
	 * 	chanB is set to 0
	 * 	one shot mode
	 * 	channels are syncs
	 */

	// LOOK AT ME
	char SYNC_CHAN = 0;
	// If SYNC_CHAN = 1 then OUT1 and OUT2 will be update at the same time:
	// 	1/ write OUT1 (OUT1 will not change)
	//	2/ write OUT2 (OUT2 will not change)
	//	3/ Wait for the next clock cycle
	//	4/ Now OUT1 and OUT2 are updated
	// for more information : https://github.com/oscimp/oscimpDigital/blob/master/doc/IP/axi_to_dac.md
	
	// LOOK AT ME
	double Vref = 2.;	// Volts
	// Vref is the voltage supply of DACs
	// You may want to change it match your configuration

	double Vq = Vref/DAC_ELEMENT_SIZE;
	axi_to_dac_full_conf("/dev/axi_to_dac", 10, 20, 0, SYNC_CHAN);

	/*******/
	/* PID */
	/*******/
	
	// This variables are used to control OUT1 of the redpitaya
	// In the case of Marion's experiement : green laser 556 nm
	double KpGreen=0; 		 		// Proportionnal gain				// LOOK AT ME
	double KiGreen=0;  				// Integral gain	 			// LOOK AT ME
	double KdGreen=0;  				// Derivative gain	     			// LOOK AT ME
	double gainGreen=0;				// Gain on error [V/THz] (see my_pi function)	// LOOK AT ME
	double mesureGreen;				// The measure from the wavelength meter
	double orderGreen = atof(argv[4]);		// The frequency in THz you want 		// LOOK AT ME
	double minGreen = -3, maxGreen = 3;		// Min and Max output voltage of the DAC	// LOOK AT ME
	double errGreen=0, errGreen_1=0, errGreen_2=0;	// Remind previous errors values (PID calculation)
	double commandGreen;				// Command send to the DAC in V
	int commandGreen_mu;				// Command send to the DAC in machin unit

	// This variables are used to control OUT2 of the redpitaya
	// In the case of Marion's experiment : blue laser 399 nm
	double KpBlue=0.4; 		 		// Proportionnal gain				// LOOK AT ME
	double KiBlue=0.;  				// Integral gain	 			// LOOK AT ME
	double KdBlue=0;  				// Derivative gain	     			// LOOK AT ME
	double gainBlue=2631.6;				// Gain on error [V/THz] (see my_pi function)	// LOOK AT ME
	double mesureBlue;				// The measure from the wavelength meter
	double orderBlue = atof(argv[5]);		// The frequency in THz you want 		// LOOK AT ME
	double minBlue = -3, maxBlue = 3;		// Min and Max output voltage of the DAC	// LOOK AT ME
	double errBlue=0, errBlue_1=0, errBlue_2=0;	// Remind previous errors values (PID calculation)
	double commandBlue;				// Command send to the DAC in V
	int commandBlue_mu;				// Command send to the DAC in machin unit

        /********/
	/* LOOP */
        /********/

	while (1){
		// Jump here if no udp package received
		aze:

		//Check if the file "stop_pid" has changed to close the socket.
		command_file = fopen("/opt/redpitaya/laser_lock_LSR/bin/stop_pid", "r");
		stop_pid = fgetc(command_file);
		fclose(command_file);
		if(stop_pid=='1'){
			printf("Closing socket\n");
			close_udp();
			break;
		}

		
		for ( udpcount = 0 ; udpcount < 2 ; udpcount ++){
			// Listen UDP
			bytes_read = recvfrom(sock,&recv_data, sizeof(double), 0, (struct sockaddr *)&server_addr, &addr_len);
			
			// LOOK AT ME
			//
			// UDP problem : We don't know if the data received is for the green or the blue laser.
			//
			// Solution : Compare the data received with the order given in argument
			//            If floor(recv_data) == floor(orderBlue) then we can guess that the reveived data
			//	      concerne the blue laser.
			//
			// TODO : We can use this limitation as a safety procedure. If a laser is lost => floor(recv_data) very different of floor(orderBlue),
			//	  we can try to stop the lock/laser to not damage the equipement.
	
			// Compare the received data with the green order
			if (floor(recv_data) == floor(orderGreen)){
				mesureGreen = recv_data;
				udppackage = 1;
	                        //printf("green laser \n");	
			}

			// Compare the received data with the blue order
			else if (floor(recv_data) == floor(orderBlue)){
				mesureBlue = recv_data;
				udppackage = 2;
			}
			// If no match then listen again
			else goto aze;
		}

		// If there is a match :
		
		/* Apply the correction */
		switch(udppackage){
			case 1:
#ifdef USE_REDPITAYA_PID
				// Apply correction to the previous command
				commandGreen += my_pi(orderGreen, mesureGreen, \
						      KpGreen, KiGreen, KdGreen, gainGreen, \
						      &errGreen, &errGreen_1, &errGreen_2);
#else
				// bypass the controller
				commandGreen = 1*gainGreen*(orderGreen - mesureGreen);
#endif
				// Born the command
				if (commandGreen > maxGreen){ 
					commandGreen=maxGreen;
					errGreen = 0;
					errGreen_1 = 0;
					errGreen_2 = 0;
				}

				else if (commandGreen < minGreen) {
					commandGreen=minGreen;
					errGreen = 0;
					errGreen_1 = 0;
					errGreen_2 = 0;
				}

				// Change command into machin unit
				if (commandGreen <= Vq) commandGreen_mu = DAC_ELEMENT_SIZE + round(commandGreen/Vq); //there was a -1 here, I think it was bugging everything
				else if (commandGreen > Vq) commandGreen_mu = round(commandGreen/Vq) - 1;  // added the -1 here so that a +1 command leads to a +1 DAC output
				
				// Send the new command to the DAC
				axi_to_dac_set_chan("/dev/axi_to_dac", CHANA, commandGreen_mu);
				
				// DEBUG
				//printf("mesure = %lf\tu = %lf\n", mesureGreen, commandGreen);
				break;

			case 2:
#ifdef USE_REDPITAYA_PID
				// Apply correction to the previous command
				commandBlue += my_pi(orderBlue, mesureBlue, \
						      KpBlue, KiBlue, KdBlue, gainBlue, \
						      &errBlue, &errBlue_1, &errBlue_2);
				
#else
				// bypass the controller
				commandBlue = 1*gainBlue*(orderBlue - mesureBlue);
#endif
				// TODO : should we do that ?
				// Born the command
				// if so reset errors
				if (commandBlue > maxBlue){
					commandBlue=maxBlue;
					errBlue = 0;
					errBlue_1 = 0;
					errBlue_2 = 0;
				}
				else if (commandBlue < minBlue){
					commandBlue=minBlue;
					errBlue = 0;
					errBlue_1 = 0;
					errBlue_2 = 0;
				}

				// Change command into machin unit
				if (commandBlue <= Vq) commandBlue_mu = DAC_ELEMENT_SIZE + round(commandBlue/Vq); //there was a -1 here, I think it was bugging everything
				else if (commandBlue > Vq) commandBlue_mu = round(commandBlue/Vq) - 1 ; // added a -1 here so that a +1 command leads to a +1 DAC output
				
				// Send the new command to the DAC
				axi_to_dac_set_chan("/dev/axi_to_dac", CHANB, commandBlue_mu);
				
				// DEBUG
				//printf("mesure = %lf\tu = %lf\n", mesureBlue, commandBlue);
				break;
			default: break;
		
		} /* end of the correction */


		// write the data in the data file
		//fd=fopen(filename, "a");
		//gettimeofday(&stop, NULL);
		//elapsed_time = (double)(stop.tv_sec - start.tv_sec)*1e6+(double)(stop.tv_usec - start.tv_usec);
		//fprintf(fd,"%24.16lf %24.16lf %24.16lf\n", elapsed_time/1e6, mesureBlue, mesureGreen);
		//fclose(fd);
		// end write data file


		// TODO : It may slow the process you may want to remove it
		// read stop_pid to see if we have to stop
		//command_file = fopen("/opt/redpitaya/laser_lock_LSR/bin/stop_pid", "r");
		//stop_pid = fgetc(command_file);
		//fclose(command_file);
		//if(stop_pid=='1'){
		//	printf("Closing socket");
		//	break;
		//}
		/* end modification */
	}
	//close_udp();
	axi_to_dac_set_chan("/dev/axi_to_dac", CHANA, 0);
	axi_to_dac_set_chan("/dev/axi_to_dac", CHANB, 0);
	return 0;
}

