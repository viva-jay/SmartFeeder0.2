#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <softPwm.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <signal.h>
#include <errno.h>
#include <MQTTClient.h>
#include <string.h>

#define MOTOR_PIN 1
#define CS_MCP3008 10
#define SPI_CHANNEL 0
#define SPI_SPEED 1000000

#define ADDRESS     "tcp://175.158.15.37:1883"
#define CLIENTID    "RaspberryClient"
#define TOPIC       "Examples"
#define PAYLOAD     "Hello World!"
#define QOS         0
#define TIMEOUT     10000L

int sigCnt = 0;
int alramflag = 1;
pthread_mutex_t mutex;

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
volatile MQTTClient_deliveryToken deliveredtoken;

void * handle_motor(void *);
void * handle_voice(void *);
void * handle_music(void *);
void * handle_publish(void *);
void * handle_subscribe(void *);

void voice_timer();
int read_voiceValue();
void mqtt_connection();
void route_message(char *);
void on_signal(int);
void on_disconnect(void *, char *);
int on_message(void *, char *, int , MQTTClient_message *);
void on_delivered(void *, MQTTClient_deliveryToken);

pthread_t motor_id,
						voice_id,
						music_id,
						send_id,
						recive_id; //thread id
						
int main(int argc, char *argv[])
{
	if(wiringPiSetup() == -1)
	{
		fprintf(stdout,"wiringPiSetup Failed : %s\n",strerror(errno));
		return 1;
	}
	if(wiringPiSPISetup(SPI_CHANNEL,SPI_SPEED) == -1)
	{
		fprintf(stdout,"wiringPiSPISetup Failed : %s\n",strerror(errno));
		return 1;
	}
	mqtt_connection();
	
	
	//pthread_create(&voice_id,NULL,handle_voice,NULL);
	//pthread_detach(voice_id); //종료기다림 여기서 중지..
		pthread_create(&recive_id,NULL,handle_subscribe,NULL);
		pthread_detach(recive_id);
	
	
	while(1);
	return 0;
}
//motor 함수 - 쓰레드
void * handle_motor(void * arg)
{
	softPwmCreate(MOTOR_PIN,0,200);
	softPwmWrite(MOTOR_PIN,5);
	delay(600);
	softPwmWrite(MOTOR_PIN,25);
	delay(600);
	digitalWrite(MOTOR_PIN, 0);
	
	return NULL;
}

void * handle_voice(void * arg)
{
	int voiceValue = 0;
	int voiceCnt=0; 
	int i;
	pinMode(CS_MCP3008,OUTPUT);
	while(1)
	{
		voiceValue = read_voiceValue();
		if(voiceValue > 1000) //1000이상인 시점 기준으로 
		{
			for(i=0; i < 20 ;i++)
			{
				printf("%d\n",voiceValue = read_voiceValue());
				if(voiceValue > 1000)
					voiceCnt++;
			}
			printf("\n");
		}
		if(voiceCnt > 15)
		{
			sigCnt++;//알람 결정 카운터.
			if(alramflag)
			{
				voice_timer();//타이머 시작.
				alramflag = 0;
			}
		}
		voiceCnt=0;
	}
	return NULL;
}
void * handle_music(void *arg)
{
	system("omxplayer -o hdmi /home/pi/Downloads/dog.mp3");
	return NULL;
}
//mqtt - send 쓰레드
void * handle_publish(void * arg)
{
	 MQTTClient_deliveryToken token;
	//payload - 송신될 메시지.
    pubmsg.payload = PAYLOAD;
    pubmsg.payloadlen = strlen(PAYLOAD);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    
    //메시지 발행.
    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);      
}
//mqtt - recv 쓰레드
void * handle_subscribe(void * arg)
{
	deliveredtoken = 0;
     MQTTClient_subscribe(client, TOPIC, QOS);
     while(1);
     MQTTClient_disconnect(client, 10000);
     MQTTClient_destroy(&client);
}
//timer
void voice_timer()
{
	struct sigaction sigAction;
	struct itimerval timer;
	
	//sigaction 구조체 초기화
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags=0;
	sigAction.sa_handler = &on_signal;
	
	//itimerval 구조체 초기화
	timer.it_value.tv_sec=60;
	timer.it_value.tv_usec =0;
	timer.it_interval.tv_sec =0;
	timer.it_interval.tv_usec=0;
	sigaction(SIGVTALRM,&sigAction,NULL);
	setitimer(ITIMER_VIRTUAL,&timer,NULL);
}
//timer handler
void on_signal(int sig)
{
	if(sig == SIGVTALRM && sigCnt  > 2000)
	{
		printf("log : send a message to server\n");
		pthread_create(&send_id,NULL,handle_publish,NULL);
		pthread_detach(send_id);
	}
	sigCnt = 0;
	alramflag =1;
}

int read_voiceValue()
{
	unsigned char adcChannel = 0;
	unsigned char buff[3];
	int adcValue = 0;
	
	buff[0] = 0x06 | ((adcChannel & 0x07) >> 2);
	buff[1] = ((adcChannel & 0x07) << 6);
	buff[2] = 0x00;
	digitalWrite(CS_MCP3008, 0);
	wiringPiSPIDataRW(SPI_CHANNEL,buff,3);

	buff[1] = 0x0F & buff[1];
	adcValue = (buff[1] << 8) & 0b1100000000;
	adcValue |= (buff[2] & 0xff);
	digitalWrite(CS_MCP3008,1);
	
	return adcValue;
}

void mqtt_connection()
{
	MQTTClient_deliveryToken token;
    int rc;
 
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
 
    MQTTClient_setCallbacks(client, NULL, on_disconnect, on_message, on_delivered);
 
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);       
    }
}
void on_delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}
int on_message(void *context, char *topic, int topicLen, MQTTClient_message *message) //메시지가 도착했을 경우 호출
{
    char *payloadMsg;
    printf("topic: %s\n", topic);
    printf("message: ");
    
    payloadMsg = message->payload;
	payloadMsg[message->payloadlen] ='\0';
	
	route_Message(payloadMsg);
    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic);
    return 1;
    
}
void on_disconnect(void *context, char *cause) //서버에 대한 연결이 끊길경우 호출
{
    printf("Connection lost : %s\n",cause);
}
void route_Message(char* message)
{
	if(!strcmp(message,"motor"))
	{
		printf("connect motor");
		pthread_create(&motor_id,NULL,handle_motor,NULL);
		pthread_detach(motor_id);
	}
	else if(!strcmp(message,"music_start"))
	{
		printf("connect music");
		pthread_create(&music_id,NULL,handle_music,NULL);
		pthread_detach(music_id); 
		
	}else if(!strcmp(message,"music_stop"))
	{
		printf("log : Music stop");
		system("kill $(pgrep omxplayer)");
	}
}

