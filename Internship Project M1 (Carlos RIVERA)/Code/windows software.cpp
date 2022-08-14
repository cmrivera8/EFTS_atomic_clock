// WavelengthDemo.cpp : Defines the entry point for the application
#include "stdafx.h"
#include "windows.h"
#include "WavelengthDemo.h"
#include "wlmData.h"
#include <stdio.h>
#include <fstream>
#include <iomanip>

//test udp
#include <sys/types.h>
#include <sys/timeb.h>
#include <winsock2.h>
#include <errno.h>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define TIMEOUT 10

/********************************************************************************/
/******************************* LOOK AT ME !!!!! *******************************/
/********************************************************************************/
#define TIME_CONSTANT 10// 80	// this is the sample time of the Wavelength meter
							// You may want to change it
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


int main(void){
	return _tWinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOW);
}

//-------------------------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// socket
	int sock;
	int addr_len, bytes_read;
	char recv_data[1024];
	struct sockaddr_in server_addr, client_addr;

	// to control blocking mode of the socket
	u_long iMode = 0;

	// measure
	union mybufLambda1 {
		double data_lambda1_d;
		char data_lambda1_c[8];
	};

	union mybufLambda2 {
		double data_lambda2_d;
		char data_lambda2_c[8];
	};

	union mybufLambda3 {
		double data_lambda3_d;
		char data_lambda3_c[8];
	};

	union mybufLambda4 {
		double data_lambda4_d;
		char data_lambda4_c[8];
	};

	union mybufLambda5 {
		double data_lambda5_d;
		char data_lambda5_c[8];
	};

	union mybufLambda6 {
		double data_lambda6_d;
		char data_lambda6_c[8];
	};

	union mybufLambda7 {
		double data_lambda7_d;
		char data_lambda7_c[8];
	};

	union mybufLambda8 {
		double data_lambda8_d;
		char data_lambda8_c[8];
	};


	// Init all channels
	double dblLambda1 = 0.0;
	double dblLambda2 = 0.0;
	double dblLambda3 = 0.0;
	double dblLambda4 = 0.0;
	double dblLambda5 = 0.0;
	double dblLambda6 = 0.0;
	double dblLambda7 = 0.0;
	double dblLambda8 = 0.0;

	long number_of_channels = 0;
	number_of_channels = GetChannelsCount(0); //!!idem doc + numb channels sur un long !!!
	while (1) {
		/* INIT SOCKET */
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			printf("Socket error : %i\n", sock);
			exit(1);
		}

		//char enable_opt = 1;
		//if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable_opt, sizeof(int)) < 0) {
		//	printf("setsockopt(SO_REUSEADDR) failed");
		//	}

		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(1234);
		server_addr.sin_addr.s_addr = INADDR_ANY;
		memset(&(server_addr.sin_zero), 0, 8);

		if (bind(sock, (struct sockaddr*) & server_addr, sizeof(struct sockaddr)) == -1) {
			printf("Bind error");
			exit(1);
		}

		addr_len = sizeof(struct sockaddr);
		printf("\nUDPServer Waiting for client on port %d \n", 1234);

		//-------------------------
		// Set the socket I/O mode: In this case FIONBIO
		// enables or disables the blocking mode for the 
		// socket based on the numerical value of iMode.
		// If iMode = 0, blocking is enabled; 
		// If iMode != 0, non-blocking mode is enabled.
		ioctlsocket(sock, FIONBIO, &iMode);
		/* END INIT SOCKET */

		ofstream fout("wavelength.log", ofstream::app);
		fout << "number_of_channels=" << number_of_channels << endl;
		fout.close();

		while (1) {
			// Listen for a connection
			printf("Wait for receive\n");
			fflush(stdout);
			iMode = 0;	// blocking mode
			ioctlsocket(sock, FIONBIO, &iMode);
			bytes_read = recvfrom(sock, recv_data, 1024, 0, (struct sockaddr*) & client_addr, &addr_len);
			iMode = 1;	// non-blocking mode
			ioctlsocket(sock, FIONBIO, &iMode);
			
			//printf("Received : %s\t%i\n", recv_data, bytes_read);
			if (memcmp(recv_data, "hello", strlen("hello")) == 0) {
				printf("Connection established\n");
			}

			memset(recv_data, 0, sizeof recv_data);

			fflush(stdout);

			// do cpp stuff
			union mybufLambda1 data1;
			union mybufLambda2 data2;
			union mybufLambda3 data3;
			union mybufLambda4 data4;
			union mybufLambda5 data5;
			union mybufLambda6 data6;
			union mybufLambda7 data7;
			union mybufLambda8 data8;

			for (;;) {

				/****************************************** README ******************************************/
				/*																							*/
				/* If you want to add a channel :														    */
				/* 	1/ Uncomment dblLambdai = GetFrequencyNum(i, 0); // i being your channel				*/
				/*	2/ Uncomment the corresponding lines to send the data									*/
				/*	3/ You may want to change the code on the redpitaya to consider this new channel		*/
				/*                                                                                          */
				/********************************************************************************************/

				// read frequency in THz
				//dblLambda1 = GetFrequencyNum(1, 0);	// Read ch1		Green laser
				dblLambda2 = GetFrequencyNum(2, 0);	// Read ch2			Blue laser
				//dblLambda3 = GetFrequencyNum(3, 0);	// Read ch3
				//dblLambda4 = GetFrequencyNum(4, 0);	// Read ch4
				//dblLambda5 = GetFrequencyNum(5, 0);	// Read ch5
				//dblLambda6 = GetFrequencyNum(6, 0);	// Read ch6
				//dblLambda7 = GetFrequencyNum(7, 0);	// Read ch7
				//dblLambda8 = GetFrequencyNum(8, 0);	// Read ch8

				// Send data to the redpitaya
				//data1.data_lambda1_d = dblLambda1;
				//sendto(sock, data1.data_lambda1_c, 8 * sizeof(char), 0, (struct sockaddr*) & client_addr, sizeof(struct sockaddr));

				data2.data_lambda2_d = dblLambda2;
				sendto(sock, data2.data_lambda2_c, 8 * sizeof(char), 0, (struct sockaddr*) & client_addr, sizeof(struct sockaddr));

				//data3.data_lambda3_d = dblLambda3;
				//sendto(sock, data3.data_lambda3_c, 8 * sizeof(char), 0, (struct sockaddr*) & client_addr, sizeof(struct sockaddr));

				//data4.data_lambda4_d = dblLambda4;
				//sendto(sock, data4.data_lambda4_c, 8 * sizeof(char), 0, (struct sockaddr*) & client_addr, sizeof(struct sockaddr));

				//data5.data_lambda5_d = dblLambda5;
				//sendto(sock, data5.data_lambda5_c, 8 * sizeof(char), 0, (struct sockaddr*) & client_addr, sizeof(struct sockaddr));

				//data6.data_lambda6_d = dblLambda6;
				//sendto(sock, data6.data_lambda6_c, 8 * sizeof(char), 0, (struct sockaddr*) & client_addr, sizeof(struct sockaddr));

				//data7.data_lambda7_d = dblLambda7;
				//sendto(sock, data7.data_lambda7_c, 8 * sizeof(char), 0, (struct sockaddr*) & client_addr, sizeof(struct sockaddr));

				//data8.data_lambda8_d = dblLambda8;
				//sendto(sock, data8.data_lambda8_c, 8 * sizeof(char), 0, (struct sockaddr*) & client_addr, sizeof(struct sockaddr));

				// Not blocking read
				// If receive "Bye" then stop for(;;) loop.
				// To stop the loop run 'echo -ne '1' > stop_lock' in the redpitaya
				bytes_read = recvfrom(sock, recv_data, 1024, 0, (struct sockaddr*) & client_addr, &addr_len);
				/*if (bytes_read > 0) {
					recv_data[3] = '\0'; recv_data[3] = '\0';
					printf("!Received : %s\n", recv_data);
					break;
				}*/

				if (memcmp(recv_data, "Bye", strlen("Bye")) == 0) {
					printf("Connection closed by Redpitaya\n");
					break; // Out of for loop, stop sending wavelength Data.
				}
				memset(recv_data, 0, sizeof recv_data);
				// DO NOT COMMENT
				Sleep(TIME_CONSTANT);	// Wait for the sample time
			}
			break; // Out of outer while loop to start socket from scratch if "Bye" received.
		}
		closesocket(sock);
		WSACleanup();
	}
	closesocket(sock);
	WSACleanup();
	return 0;
}
