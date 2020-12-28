#include <arpa/inet.h>
#include <inttypes.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SERVER_PORT 5430
#define MC_DATA_PORT_1 5450
#define MC_STREAM_PORT_1 5451
#define MC_DATA_PORT_2 5452
#define MC_STREAM_PORT_2 5453
#define MC_LIVE_STREAM_PORT 5454
#define BUF_SIZE 1024

int pauseFlag = false;
char *ifName = "wlp3s0";
int currentStation = 1;
int streamFlag = 0;
double tSec = 1.0;

struct stationInfo {
    uint8_t stationNumber;
    uint8_t stationNameSize;
    char stationName[200];
    char multicastAddr[16];
    uint16_t dataPort;
    uint16_t infoPort;
    uint32_t bitRate;
};

struct siteInfo {
    uint8_t type;
    uint8_t siteNameSize;
    char siteName[200];
    uint8_t siteDescSize;
    char siteDesc[200];
    uint8_t stationCount;
    struct stationInfo stationList[2];
};

struct data {
    char buffer[BUF_SIZE];
};

struct streamInfo {
    char songName[200];
    char nextSongName[200];
    int secLeft;
};

struct receiverArguments {
    int socket;
    struct sockaddr_in sin;
    int sLength;
};

void
delay(int t)
{
    clock_t now = 0, then = 0;

    long pause = t * (CLOCKS_PER_SEC / 1000000);
    now = then = clock();
    while ((now - then) < pause) {
        now = clock();
    }
}

// Function for receiving stream info.
void *
receiveStreamInfo(struct receiverArguments *rStreamStruct)
{
    struct receiverArguments recArgs = {0};
    recArgs.socket = rStreamStruct->socket;
    recArgs.sin = rStreamStruct->sin;
    recArgs.sLength = rStreamStruct->sLength;

    struct streamInfo sInfo = {0};
    bool flag = true;
    bool flag2 = false;
    int nbrecv = 0;
    int count = 0;
    int length = sizeof(recArgs.sin);
    while (flag == true) {
        if (streamFlag == 1) {
            break;
        }

        memset(sInfo.songName, '\0', sizeof(sInfo.songName));
        memset(sInfo.nextSongName, '\0', sizeof(sInfo.nextSongName));
        sInfo.secLeft = 0;

        if ((nbrecv = recvfrom(recArgs.socket, &sInfo, sizeof(sInfo), 0,
                               (struct sockaddr *)&recArgs.sin, &length)) < 0) {
            fprintf(stderr, "fail while receiving data! \n");
            exit(-1);
            return 0;
        }
        printf("Stream Info Received!!\n");
        printf("Song Name:      %s\n", sInfo.songName);
        printf("Next Song Name: %s\n", sInfo.nextSongName);
        printf("Seconds Left:   %d\n\n", sInfo.secLeft);
        usleep(100000);
    }

    close(recArgs.socket);
}

void *
receiveLiveData(struct receiverArguments *recArgs)
{
    struct receiverArguments recDataStruct = {0};
    recDataStruct.socket = recArgs->socket;
    recDataStruct.sin = recArgs->sin;
    recDataStruct.sLength = recArgs->sLength;

    struct data d = {0};
    FILE *fp = fopen("output.webm", "w");
    // Can recieve any kind of file (txt, jpg, mp4)
    if (fp == NULL) {
        perror("Error has occurred. File could not be opened");
        exit(-1);
    }

    bool flag = true;
    bool flag2 = false;
    int nbrecv = 0;
    int count = 0;
    int packetIndex = 0;
    int length = sizeof(recDataStruct.sin);

    clock_t start = clock();

    while (flag == true) {
        memset(d.buffer, '\0', sizeof(d.buffer));
        if (pauseFlag == 0) {
            if ((nbrecv = recvfrom(recDataStruct.socket, &d, sizeof(d), 0,
                                   (struct sockaddr *)&recDataStruct.sin,
                                   &length)) < 0) {
                fprintf(stderr, "fail while receiving data! \n");
                exit(-1);
                return 0;
            }

            count += nbrecv;
            packetIndex++;
            fwrite(d.buffer, 1, nbrecv, fp);
            delay(1000);
        } else {
            printf("Transmission Paused!!\n");
            fclose(fp);
            // fp = fopen("output.mp3", "w");
            // fclose(fp);
            break;
        }
    }

    printf("Packets received: %d\n", packetIndex);
    close(recDataStruct.socket);
}

// Function for receiving data.
void *
receiveData(struct receiverArguments *temp)
{
    clock_t end = 0;
    double executionTime = 0.0;
    struct receiverArguments rDataStruct;
    rDataStruct.socket = temp->socket;
    rDataStruct.sin = temp->sin;
    rDataStruct.sLength = temp->sLength;

    struct data d = {0};
    FILE *fp = NULL;
    fp = fopen("output.mp3",
               "w"); // Can recieve any kind of file (txt, jpg, mp4)
    if (fp == NULL) {
        perror("Error has occurred. File could not be opened");
        exit(-1);
    }

    bool flag = true;
    bool flag2 = false;
    int nbrecv = 0;
    int count = 0;
    int packetIndex = 0;
    int length = sizeof(rDataStruct.sin);

    clock_t start = clock();

    while (flag == true) {
        memset(d.buffer, '\0', sizeof(d.buffer));
        if (pauseFlag == 0) {
            if ((nbrecv = recvfrom(rDataStruct.socket, &d, sizeof(d), 0,
                                   (struct sockaddr *)&rDataStruct.sin,
                                   &length)) < 0) {
                fprintf(stderr, "fail while receiving data! \n");
                exit(-1);
                return 0;
            }

            end = clock();
            executionTime = (double)(start - end) / CLOCKS_PER_SEC;

            if (executionTime > tSec) {
                fclose(fp);
                fp = fopen("output.mp3", "w");
                start = clock();
            }

            count += nbrecv;
            // printf("Packet Number: %i\n\n",packet_index);
            packetIndex++;
            fwrite(d.buffer, 1, nbrecv, fp);
        } else {
            printf("Transmission Paused!!\n");
            fclose(fp);
            fp = fopen("output.mp3", "w");
            fclose(fp);
            break;
        }
    }

    printf("Packets received: %d\n", packetIndex);
    close(rDataStruct.socket);
}

struct receiverArguments
setUpReceiver(char *mcast_addr, int tempPort)
{
    struct sockaddr_in sin;
    struct ifreq ifr;
    struct ip_mreq mcastReq;
    int s, reuse = 1, len = 0;
    char buf[BUF_SIZE];
    int sLength = sizeof(sin);

    printf("Receiver connected to Multicast Group1: %s\n", mcast_addr);
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("receiver: socket");
        exit(1);
    }

    // Assign the respective values to the udp family structure.
    memset((char *)&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(tempPort);

    // Use the interface specified
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifName, sizeof(ifName) - 1);

    // For Reusing the same port for udp transmission.
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) <
        0) {
        perror("setting SO_REUSEADDR");
        close(s);
        exit(1);
    }

    if ((setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr,
                    sizeof(ifr))) < 0) {
        perror("receiver: setsockopt() error");
        close(s);
        exit(1);
    }

    // Binding to Socket.
    if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
        perror("receiver: bind()");
        exit(1);
    }

    mcastReq.imr_multiaddr.s_addr = inet_addr(mcast_addr);
    mcastReq.imr_interface.s_addr = htonl(INADDR_ANY);

    // Adding Membership to Multicast Group.
    if ((setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mcastReq,
                    sizeof(mcastReq))) < 0) {
        perror("mcast join receive: setsockopt()");
        exit(1);
    }
    printf("Ready to listen!\n");

    // Creating Structure and passing values required for transmission.
    struct receiverArguments recArg;
    recArg.socket = s;
    recArg.sin = sin;
    recArg.sLength = sLength;

    return recArg;
}

void
connectToMulticastGroup(char *mcast_addr)
{
    struct receiverArguments rDataStruct, rStreamStruct, rLiveStruct;
    int dataPort = 0, streamPort = 0;
    if (strcmp(mcast_addr, "225.1.1.1") == 0) {
        dataPort = MC_DATA_PORT_1;
        streamPort = MC_STREAM_PORT_1;
    } else if (strcmp(mcast_addr, "225.2.1.1") == 0) {
        dataPort = MC_DATA_PORT_2;
        streamPort = MC_STREAM_PORT_2;
    } else if (strcmp(mcast_addr, "225.3.1.1") == 0) {
        dataPort = MC_LIVE_STREAM_PORT;
    }

    if (strcmp(mcast_addr, "225.3.1.1") == 0) {
        rLiveStruct = setUpReceiver(mcast_addr, dataPort);
        pthread_t thread_id3;
        pthread_create(&thread_id3, NULL, receiveLiveData, &rLiveStruct);
    } else {
        rDataStruct = setUpReceiver(mcast_addr, dataPort);
        rStreamStruct = setUpReceiver(mcast_addr, streamPort);
        pthread_t thread_id1, thread_id2;
        pthread_create(&thread_id1, NULL, receiveStreamInfo, &rStreamStruct);
        pthread_create(&thread_id2, NULL, receiveData, &rDataStruct);

        char c1, c2;
        int temp;
        while (1) {
            scanf("%c", &c1);
            if (c1 == 'p') {
                pauseFlag = 1;
                usleep(10);
            }

            if (c1 == 'r') {
                pauseFlag = 0;
                usleep(10);
                // printf("Before creating structure!\n");
                rDataStruct = setUpReceiver(mcast_addr, dataPort);
                // printf("After creating structure!\n");
                // rStreamStruct = setUpReceiver(mcast_addr, streamPort);
                // pthread_create(&thread_id1, NULL, receiveStreamInfo,
                // &rStreamStruct);
                pthread_create(&thread_id2, NULL, receiveData, &rDataStruct);
            }

            if (c1 == 's') {
                if (currentStation == 1) {
                    temp = 2;
                }
                if (currentStation == 2) {
                    temp = 1;
                }

                printf("diff station\n");
                pauseFlag = 1;
                streamFlag = 1;
                usleep(10);

                pthread_cancel(thread_id1);
                pthread_cancel(thread_id2);

                // temp = (int)c2 - (int)('0');
                if (temp == 1) {
                    dataPort = MC_DATA_PORT_1;
                    streamPort = MC_STREAM_PORT_1;
                    strcpy(mcast_addr, "225.1.1.1");
                }

                if (temp == 2) {
                    dataPort = MC_DATA_PORT_2;
                    streamPort = MC_STREAM_PORT_2;
                    strcpy(mcast_addr, "225.2.1.1");
                }

                currentStation = temp;
                rDataStruct = setUpReceiver(mcast_addr, dataPort);
                rStreamStruct = setUpReceiver(mcast_addr, streamPort);

                pauseFlag = 0;
                streamFlag = 0;
                printf("station changed!\n");
                pthread_create(&thread_id1, NULL, receiveStreamInfo,
                               &rStreamStruct);
                pthread_create(&thread_id2, NULL, receiveData, &rDataStruct);
                // }
                // break;
                //}
            }
        }
    }
}

int
main(int argc, char *argv[])
{

    struct sockaddr_in sin;
    struct hostent *hp;
    char *host;
    char buf[BUF_SIZE];
    int s, len;
    int serverPort;

    if (argc == 3) {
        host = argv[1];
        sscanf(argv[2], "%d", &serverPort);
    } else {
        fprintf(stderr, "usage: simplex-talk host\n");
        exit(1);
    }
    system("cp input1.mp3 output.mp3");
    /* translate host name into peer's IP address */
    hp = gethostbyname(host);
    if (!hp) {
        fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
        exit(1);
    } else
        printf("Client's remote host: %s\n", argv[1]);
    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(serverPort);
    /* active open */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    } else
        printf("Client created socket.\n");

    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    } else
        printf("Client connected.\n");

    struct siteInfo site1;
    len = recv(s, &site1, sizeof(site1), 0);

    printf("Station list received: \n");
    printf("Station Number:    %u\n", site1.stationList[0].stationNumber);
    printf("Station Name Size: %u\n", site1.stationList[0].stationNameSize);
    printf("Multicast Name:    %s\n", site1.stationList[0].stationName);
    printf("Multicast Address: %s\n", site1.stationList[0].multicastAddr);
    printf("Data Port:         %u\n", site1.stationList[0].dataPort);
    printf("Info Port:         %u\n", site1.stationList[0].infoPort);
    printf("Bit Rate:          %u\n\n", site1.stationList[0].bitRate);

    printf("Station Number:    %u\n", site1.stationList[1].stationNumber);
    printf("Station Name Size: %u\n", site1.stationList[1].stationNameSize);
    printf("Multicast Name:    %s\n", site1.stationList[1].stationName);
    printf("Multicast Address: %s\n", site1.stationList[1].multicastAddr);
    printf("Data Port:         %u\n", site1.stationList[1].dataPort);
    printf("Info Port:         %u\n", site1.stationList[1].infoPort);
    printf("Bit Rate:          %u\n\n", site1.stationList[1].bitRate);

    printf("Station Number: 3\n");
    printf("Description: Live Stream\n");
    printf("Multicast Address: 225.3.1.1\n");
    printf("Data Port: %d\n", MC_LIVE_STREAM_PORT);

    int receivedStation[site1.stationCount];
    int stationSelect, flag = 0;

    while (flag == 0) {
        printf("Select Station Number: ");
        scanf("%d", &stationSelect);
        if (stationSelect == 3) {
            connectToMulticastGroup("225.3.1.1");
            flag = 1;
            break;
        } else {
            for (int i = 0; i < site1.stationCount; i++) {
                if (site1.stationList[i].stationNumber == stationSelect) {
                    close(s);
                    connectToMulticastGroup(
                        &site1.stationList[i].multicastAddr);
                    flag = 1;
                    break;
                }
            }
            if (flag == 0) {
                printf("No Such Station Exists!\n");
            }
        }
    }
}
