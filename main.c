//Title:		Untitled One
//Files:		main.c
//				UntitledOne (executable)
//Description:	Untitled One in OpenGL for Linux. Player controls a craft with 
//				lunar lander type controls and attempts to shoot and destroy
//				various targets without crashing.
//Author:		PHilip RUshik
//Contributors:	
//License:		Beerware Rev. 43H
//License Text:	
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 43H):
 * <prushik@gmail.com> wrote this file. As long as you retain this notice, you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer or soju in return. Philip Rushik
 * ----------------------------------------------------------------------------
 */


#include <stdio.h>
#ifdef __unix__
	#include <unistd.h>
#else
	#include <windows.h>
#endif
#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <math.h>
#ifndef SLOPPYTIME
	#include <sys/time.h>
	#include <time.h>
#endif
#ifndef NONETWORK
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#endif
#ifdef AUDIO
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alut.h>
#endif

#define width 600.0f
#define height 600.0f
#define w2 width*2.0f
#define h2 height*2.0f
#define PI 3.1415923f
#define false 0
#define true 1

#define VK_W 'w'
#define VK_A 'a'
#define VK_D 'd'
#define VK_E 'e'
#define VK_UP GLUT_KEY_UP+256
#define VK_LEFT GLUT_KEY_LEFT+256
#define VK_DOWN GLUT_KEY_DOWN+256
#define VK_RIGHT GLUT_KEY_RIGHT+256
#define VK_RETURN 13
#define VK_SPACE 32
#define VK_ESC 27

struct menu
{
       short  option, rot;
       int    help;
       double r,g,b;
};

struct craft
{
		float  x,y,
			  xspeed, yspeed;
		float  r1,r2,r3,
			  g1,g2,g3,
			  b1,b2,b3;
		float  health;
		short  dir, reload, range, prime;
		int   land, visible;
		GLuint list;
};

struct bulletType1
{
	float	x,y,
		xspeed, yspeed;
	short	dir, spin, active, life;
	int	visible;
};

struct effect
{
       short type, life;
       float x,y,
             xspeed,yspeed,
             r,g,b;
       int  visible;
};

struct asteroids
{
       short size, rot;
       float x,y,
             xspeed,yspeed,
             r,g,b;
       int  visible;
};

struct targets
{
       short size, rot;
       float x,y,
             xspeed,yspeed,
             r,g,b;
       int  visible;
};

struct stats
{
	int timer,end;
	int p1dam,p2dam,prdam;
	int p1shots,p2shots,prshots;
	int targets,asteroids;
};

#ifndef NONETWORK
	int ndam=0,nfire=0,lsnfire=0;
#endif

void GameInit();
void AsteroidSetup();
void TargetSetup();
void GameMain();
void Fire(struct craft craft);
void Crash(struct craft craft);
void Move();
void TargetStep();
void AsteroidStep();
void Collisions();
int ShipXGround(struct craft craft);
int Bullet1XShip(struct bulletType1 b, struct craft craft);
int AsteroidXShip(struct asteroids a, struct craft craft);
int BulletXAsteroid(struct bulletType1 b, struct asteroids a);
int BulletXTarget(struct bulletType1 b, struct targets a);
GLuint BuildShip(struct craft craft);
void BuildTarget();
void DrawAll();
void DrawShip(struct craft craft);
void DrawBullets();
void DrawGround();
void DrawEffects();
void DrawAsteroids();
void DrawTargets();
void Thrust(struct craft craft);
void Hit(struct bulletType1 bt1);
void Keys();
int XLines(double x11, double y11, double x12, double y12, double x21, double y21, double x22, double y22);
inline void ShowText(unsigned char *);
inline int KeyState(int);
void DrawStats();

void MenuInit();
void MenuMain();

void HelpSetup();
void HelpMain();

void NetworkSetup();

inline int KeyState(int);
void PlaySounds();

//Game Structures... Sorry that they are all global.
struct menu menu;					//Menu structure
struct craft user;					//Player 1's ship
struct craft user2;				//Player 2's ship
struct craft remote;				//Network player's ship
struct asteroids asteroid[50];	//Asteroids
struct targets target[10];			//Targets
struct bulletType1 bt1[25];		//Bullets
struct effect effect[512];		//Particle effects
struct stats status;				//Game statistics

void (*CurrentLoop)() = MenuInit;

//Variables... Sorry that they are all global.
static double fps = 30;			//Frames per second
static short bt1C = 0;				//Count of active bullets
static short effectC = 0;			//Count of particles
static short aC = 0;
static short aN = 0;
static double gravity = -0.095;	//Gravity strength
static double thrust = 0.25;		//Ship thrust strength
static GLuint target_list;			//Display list for targets

static volatile int key[512];

//Variables needed for network communication
#ifndef NONETWORK
	int sock, length;
	struct sockaddr_in server, client;
	struct hostent *hp;
	unsigned char buffer[1024];
	unsigned char opponentIP[256];
#endif

//Variables needed for OpenAL
#ifdef AUDIO
	static	ALuint buffers[25];
	static	ALfloat sourcePos[3],sourceVel[3],sourceOri[3],
			listenerPos[3],listenerVel[3],listenerOri[3];
	static	ALuint source[30];
#endif

//Wrapper function to make my old Windows crap work with GLUT
void run()
{
	#ifdef INSTRUMENT
		int timea,timeb,timedelta;
		timea=glutGet(GLUT_ELAPSED_TIME);
		printf("%d : ",timea);
	#endif

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	#ifdef INSTRUMENT
		timeb=glutGet(GLUT_ELAPSED_TIME);
		printf("%d : ",timeb-timea);
	#endif

	Keys();

	#ifdef INSTRUMENT
		timea=glutGet(GLUT_ELAPSED_TIME);
		printf("%d : ",timea-timeb);
	#endif

	#ifndef SLOPPYTIME
		struct timeval a;
		struct timeval b;

		gettimeofday(&a, 0);
	#endif


	#ifndef NONETWORK
		if (status.prdam!=-1)
		{
			int buflen;
			sprintf(buffer,"%f|%lf|%hd|%lf|%lf|%d|%d%n",(float)-user.x,user.y,180-user.dir,-user.xspeed,user.yspeed,ndam,nfire,&buflen);
			sendto(sock,buffer,buflen+1,0,(struct sockaddr *) &server,length);
			recvfrom(sock,buffer,buflen+1,0,(struct sockaddr *) &server,&length);
			sscanf(buffer,"%f|%f|%hd|%f|%f|%d|%d",&remote.x,&remote.y,&remote.dir,&remote.xspeed,&remote.yspeed,&ndam,&lsnfire);
			if (lsnfire==1)
			{
				Fire(remote);
				lsnfire=0;
				status.prshots+=1;
			}
			nfire=0;
		}
	#endif
	#ifndef NONETWORK
//		if (status.prshots==-1)
	#endif

	#ifdef INSTRUMENT
		timeb=glutGet(GLUT_ELAPSED_TIME);
		printf("%d : ",timeb-timea);
	#endif

	CurrentLoop();


	#ifdef INSTRUMENT
		timea=glutGet(GLUT_ELAPSED_TIME);
		printf("%d : ",timea-timeb);
	#endif

	#ifndef SLOPPYTIME
		gettimeofday(&b, 0);

		if (((1000/fps)*1000)>(b.tv_usec-a.tv_usec))
			#ifdef __unix__
				usleep(((1000/fps)*1000)-(b.tv_usec - a.tv_usec));
			#else
				Sleep(1000/fps);
			#endif
	#else
		#ifdef __unix__
			usleep(((1000/fps)*1000));
		#else
			Sleep(1000/fps);
		#endif
	#endif

	#ifdef INSTRUMENT
		timeb=glutGet(GLUT_ELAPSED_TIME);
		printf("%d\n",timeb-timea);
	#endif

	return;
}

//Key Press Callback
void keydown(unsigned char k, int x, int y)
{
	if(key[k]==false)
		key[k]=2;
	else
		key[k]=true;

	return;
}

//Key Release Callback
void keyup(unsigned char k, int x, int y)
{
	key[k]=false;

	return;
}

//Special Key Press Callback
void skeydown(int k, int x, int y)
{
	if (key[k+256]==false)
		key[k+256]=2;
	else
		key[k+256]=true;

	return;
}

//Special Key Release Callback
void skeyup(int k, int x, int y)
{
	key[k+256]=false;

	return;
}

int main(int argc, char * argv[])
{
	//This line causes a failure on some video cards
//	glutInitContextVersion (2, 1);

	#ifdef AUDIO
		// Init openAL
		alutInit(0, NULL);

		int error;
		// Create the buffers
		alGenBuffers(25, buffers);

		char whistle_sound[3]={0x7F,0x8F,0x7F};
		int i;
		unsigned char noise[1000];
		for (i=0;i<1000;i++)
			noise[i]=0x7F+((rand()%6)-3);
		unsigned char boom[1000];
		for (i=0;i<1000;i++)
			boom[i]=0x7F+(((rand()%24)-12))*((1000-i)/500);
		alBufferData(buffers[0],AL_FORMAT_MONO8,noise,1000,4000);
		alBufferData(buffers[1],AL_FORMAT_MONO8,noise,1000,2000);
		alBufferData(buffers[2],AL_FORMAT_MONO8,boom,1000,2000);

		// Generate the sources
		alGenSources(30, source);

		for (i=0;i<25;i++)
		{
			alSourcei(source[i], AL_BUFFER, buffers[0]);

			alSourcefv(source[i], AL_POSITION, sourcePos);
			alSourcefv(source[i], AL_VELOCITY, sourceVel);
			alSourcefv(source[i], AL_DIRECTION, sourceOri);
			alSourcei(source[i],AL_LOOPING,true);
		}

		alSourcei(source[25],AL_BUFFER,buffers[2]);
		alSourcei(source[26],AL_BUFFER,buffers[2]);
		alSourcei(source[27],AL_BUFFER,buffers[1]);
		alSourcei(source[27],AL_LOOPING,true);

		alListenerfv(AL_POSITION,listenerPos);
		alListenerfv(AL_VELOCITY,listenerVel);
		alListenerfv(AL_ORIENTATION,listenerOri);
	#endif

	// initialize GLUT
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );

	// set RGBA mode with double and depth buffers
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
	glutCreateWindow("Untitled One");

	// Use fullscreen only on compile-time (It's not ready)
	#ifndef FULLSCREEN
		glutReshapeWindow((int)width, (int)height);
	#else
		glutFullScreen();
	#endif

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_POLYGON_STIPPLE);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Use fullscreen only on compile-time (It's not ready)
	#ifndef FULLSCREEN
		gluPerspective(45, 1, 0.0001, 20);
	#else
		gluPerspective(45, 1.778, 0.0001, 20);
	#endif

	//Only jump to network mode if networking is enabled
	#ifndef NONETWORK
	if (argc>1)
	{
		sprintf(opponentIP,"%s",argv[1]);
		CurrentLoop=NetworkSetup;
	}
	#endif

	gluLookAt(0,0,1,0,0,0,0,1,0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//Set up callbacks
	glutDisplayFunc(run);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(skeydown);
	glutSpecialUpFunc(skeyup);
	glutIdleFunc(run);
	glClearColor(0.2,0.0,0.2,0.0);

	glutMainLoop();

	return 0;
}

inline int KeyState(int i)
{
	return key[i];
}

void ResetStats()
{
	status.timer=0;
	status.end=0;
	status.p1dam=0;
	status.p2dam=0;
	status.p1shots=0;
	status.p2shots=0;
	status.prshots=-1;
	status.prdam=-1;
	status.targets=0;
	status.asteroids=0;

	return;
}

void GameInit()
{
	if (KeyState(VK_RETURN) || KeyState(VK_SPACE))
		return;

	ResetStats();
	status.targets=-1;
	status.asteroids=-1;
	status.prshots=-1;
	status.prdam=-1;

	user.x=-200;
	user.y=-205;
	user.xspeed=0;
	user.yspeed=0;
	user.dir=90;
	user.health=100;
	user.visible=true;
	user.r1=0.3; user.g1=0.3; user.b1=1;
	user.r2=0; user.g2=0; user.b2=0.4;
	user.r3=0; user.g3=0; user.b3=0.8;
	user.range=240;
	user.prime=30;

	user2.x=200;
	user2.y=-205;
	user2.xspeed=0;
	user2.yspeed=0;
	user2.dir=90;
	user2.health=100;
	user2.visible=true;
	user2.r1=8; user2.g1=0.1; user2.b1=0.1;
	user2.r2=0.3; user2.g2=0.0; user2.b2=0;
	user2.r3=0.6; user2.g3=0.0; user2.b3=0;
	user2.range=240;
	user2.prime=30;

	user.list = BuildShip(user);
	user2.list = BuildShip(user2);

	remote.visible=false;

	int i;
	for (i=0; i<10; i++)
		target[i].visible=false;

	for (i=0; i<50; i++)
		asteroid[i].visible=false;

	CurrentLoop=GameMain;

	return;
}

void DestroyBullet(int b)
{
	bt1[b].visible=false;
	#ifdef AUDIO
		alSourceStop(source[b]);
	#endif

	return;
}

void TargetSetup()
{
	if (KeyState(VK_RETURN) || KeyState(VK_SPACE))
		return;

	int i;
	for (i=0; i<10; i++)
		target[i].visible=true;

	for (i=0; i<50; i++)
		asteroid[i].visible=false;

	ResetStats();
	status.p2dam=-1;
	status.p2shots=-1;
	status.asteroids=-1;
	status.prshots=-1;
	status.prdam=-1;

	user.x=-200;
	user.y=-205;
	user.xspeed=0;
	user.yspeed=0;
	user.dir=90;
	user.health=100;
	user.visible=true;
	user.r1=0.5; user.g1=1; user.b1=0.3;
	user.r2=0.1; user.g2=0.4; user.b2=0;
	user.r3=0.3; user.g3=0.8; user.b3=0;
	user.range=240;
	user.prime=30;

	user2.visible=false;
	remote.visible=false;

	user.list = BuildShip(user);

	if (!glIsList(target_list))
		BuildTarget();

	target[0].x=200;
	target[0].y=200;

	target[1].x=-200;
	target[1].y=200;

	target[2].x=100;
	target[2].y=100;

	target[3].x=-100;
	target[3].y=100;

	target[4].x=0;
	target[4].y=0;

	target[5].x=0;
	target[5].y=200;

	target[6].x=100;
	target[6].y=200;

	target[7].x=-100;
	target[7].y=200;

	target[8].x=0;
	target[8].y=100;

	target[9].x=0;
	target[9].y=-200;

	CurrentLoop=GameMain;

	return;
}

void AsteroidSetup()
{
	if (KeyState(VK_RETURN) || KeyState(VK_SPACE))
		return;

	int i;
	for (i=0; i<50; i++)
		asteroid[i].visible=false;

	for (i=0; i<10; i++)
		target[i].visible=false;

	ResetStats();
	status.targets=-1;
	status.p2dam=-1;
	status.p2shots=-1;
	status.prshots=-1;
	status.prdam=-1;

	user.x=-200;
	user.y=-205;
	user.xspeed=0;
	user.yspeed=0;
	user.dir=90;
	user.health=100;
	user.visible=true;
	user.r1=0.3; user.g1=0.5; user.b1=1;
	user.r2=0.1; user.g2=0.1; user.b2=0.4;
	user.r3=0.3; user.g3=0.3; user.b3=0.8;
	user.range=240;
	user.prime=10;

	user2.visible=false;
	remote.visible=false;

	user.list = BuildShip(user);

	rand(); //Randomize
	asteroid[aC].x=0;
	asteroid[aC].y=200;
	asteroid[aC].xspeed=4*((float)rand()/RAND_MAX);
	asteroid[aC].yspeed=0;
	asteroid[aC].visible=true;
	asteroid[aC].size=2;

	aN=1;
	aC++;
	if (aC>=50)
		aC=0;

	CurrentLoop=GameMain;

	return;
}

void NetworkSetup()
{
	if (KeyState(VK_RETURN) || KeyState(VK_SPACE))
		return;

	#ifndef NONETWORK

		ResetStats();
		status.targets=-1;
		status.asteroids=-1;
		status.p2shots=-1;
		status.p2dam=-1;
		status.prshots=0;
		status.prdam=0;

		user.x=-200;
		user.y=-205;
		user.xspeed=0;
		user.yspeed=0;
		user.dir=90;
		user.health=100;
		user.visible=true;
		user.r1=0.3; user.g1=0.3; user.b1=1;
		user.r2=0; user.g2=0; user.b2=0.4;
		user.r3=0; user.g3=0; user.b3=0.8;
		user.range=240;
		user.prime=30;

		remote.x=200;
		remote.y=-205;
		remote.xspeed=0;
		remote.yspeed=0;
		remote.dir=90;
		remote.health=100;
		remote.visible=true;
		remote.r1=0.8; remote.g1=0.8; remote.b1=0.1;
		remote.r2=0.3; remote.g2=0.3; remote.b2=0;
		remote.r3=0.6; remote.g3=0.6; remote.b3=0;
		remote.range=240;
		remote.prime=30;

		int i;
		for (i=0; i<10; i++)
			target[i].visible=false;

		for (i=0; i<50; i++)
			asteroid[i].visible=false;

		//Set up and open datagram socket
		sock=socket(AF_INET, SOCK_DGRAM, 0);

		//Setup socket properties
		server.sin_family = AF_INET;
		//We need to set to INADDR_ANY only if we are to be a server. Otherwise it should be our opponent's IP.
//		server.sin_addr.s_addr=inet_addr(opponentIP);
//		server.sin_addr.s_addr=INADDR_ANY;
		server.sin_addr.s_addr=htonl(INADDR_ANY);
		server.sin_port = htons(1414);

		length = sizeof(struct sockaddr_in);

		//This shit is for the select() function
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(sock,&rfds);
		struct timeval timeout;
		timeout.tv_sec=10;
		timeout.tv_usec=0;

		//Send out our discover packet
		sprintf(buffer,"UntitledOneNetworkDiscover\nStartGame?");
		sendto(sock,buffer,40,0,(struct sockaddr *) &server,length);

		if (select(sock+1,&rfds,NULL,NULL,&timeout)!=0)
		{
			recvfrom(sock,buffer,256,0,(struct sockaddr *) &server,&length);
			printf("Recieved reply from server, becoming a client\n");
		}
		else
		{

		shutdown(sock,2);

		sock=socket(AF_INET, SOCK_DGRAM, 0);

		//Setup socket properties
		server.sin_family = AF_INET;
		//We need to set to INADDR_ANY only if we are to be a server. Otherwise it should be our opponent's IP.
		server.sin_addr.s_addr=inet_addr(opponentIP);
//		server.sin_addr.s_addr=INADDR_ANY;
//		server.sin_addr.s_addr=htonl(INADDR_ANY);
		server.sin_port = htons(1414);

		length = sizeof(struct sockaddr_in);



		//Using a select statement here improperly. I just want to implement a timeout
		//Wait for a few seconds to see if I get a response from the server, else become a server
		//They say to never use a timeout, but really, I think it makes sense here
		//using timeouts makes a lot of sense for applications that need to execute in
		//real-time, or something close to real-time.

			bind(sock,(struct sockaddr *)&server,length);
			printf("Server 없어요. server됬어요.\n");
//			server.sin_addr.s_addr=INADDR_ANY;
//			bind(sock,(struct sockaddr *)&server,length);
//			while (select(1,&rfds,NULL,NULL,&timeout)<=0)
//			{
				printf("listening...\n");
				recvfrom(sock,buffer,256,0,(struct sockaddr *) &server,&length);
//			}
			sprintf(buffer,"UntitledOneNetworkConnection\nStartGame");
			sendto(sock,buffer,42,0,(struct sockaddr *) &server,length);
		}

		CurrentLoop=GameMain;

	#endif

	return;
}

void GameMain()
{
	Move();

	DrawAll();

	return;
}

void Fire(struct craft craft)
{
	bt1[bt1C].x=craft.x;
	bt1[bt1C].y=craft.y;
	bt1[bt1C].dir=craft.dir;
	bt1[bt1C].xspeed=craft.xspeed+(cos(bt1[bt1C].dir*(PI/180))*5);
	bt1[bt1C].yspeed=craft.yspeed+(sin(bt1[bt1C].dir*(PI/180))*5);
	bt1[bt1C].active=craft.prime;
	bt1[bt1C].life=craft.range;
	bt1[bt1C].visible=true;

	bt1C++;
	if (bt1C>=25)
		bt1C+=-25;

	return;
}


void Crash(struct craft craft)
{
	int i;
	for (i=0; i<100; i++)
	{
		effect[effectC].visible=true;
		effect[effectC].life=(short)(175+(75*((float)rand()/RAND_MAX)));
		effect[effectC].r=10.0*((float)rand()/RAND_MAX);
		effect[effectC].g=1.0*((float)rand()/RAND_MAX);
		effect[effectC].b=0.1;
		effect[effectC].x=craft.x+(rand()%8)-4;
		effect[effectC].y=craft.y+(rand()%8)-4;

		effect[effectC].xspeed=-cos(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
		effect[effectC].xspeed+=craft.xspeed;
		effect[effectC].yspeed=-sin(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
		effect[effectC].yspeed+=craft.yspeed;

		effectC++;
		if (effectC>500)
			effectC=0;
	}

	return;
}


void Move()
{
	Collisions();
	#ifdef AUDIO
		PlaySounds();
	#endif

	//Status-
	if (status.timer>=0)
		status.timer++;

	//BULLETS-
	int i;
	for (i=0; i<25; i++)
	{
		//TYPE 1-
		if (bt1[i].visible)
		{
			bt1[i].yspeed+=gravity;
			bt1[i].x+=bt1[i].xspeed;
			bt1[i].y+=bt1[i].yspeed;
			bt1[i].active+=-1;
			bt1[i].life+=-1;
			if (bt1[i].life<0)
				DestroyBullet(i);
//				bt1[i].visible=false;

			bt1[i].dir=(short)(atan2(bt1[i].y-(bt1[i].y-bt1[i].yspeed), bt1[i].x-(bt1[i].x-bt1[i].xspeed))*(180/PI));
			if (bt1[i].active<0 && bt1[i].y>-(height/2-75))
			{
				effect[effectC].visible=true;
				effect[effectC].life=25;
				effect[effectC].r=0.1;
				effect[effectC].g=0.1;
				effect[effectC].b=0.1;
				effect[effectC].x=bt1[i].x+(rand()%4)-2;
				effect[effectC].y=bt1[i].y+(rand()%4)-2;

				effect[effectC].xspeed=0;
				effect[effectC].yspeed=0;

				effectC++;
				if (effectC>500)
					effectC=0;
			}
		}
	}

     //USER CRAFT-
     if (user.land)
     {
        if (user.yspeed<0)
           user.yspeed=0;

        user.xspeed=0;
        user.dir=90;
     }
     else
     {
        user.yspeed+=gravity;
     }

     if (user.visible)
     {
        user.x+=user.xspeed;
        user.y+=user.yspeed;
        user.reload+=-1;
     }

	if (user2.visible)
	{
		//PLAYER 2 CRAFT-
		if (user2.land)
		{
			if (user2.yspeed<0)
				user2.yspeed=0;

			user2.xspeed=0;
			user2.dir=90;
		}
		else
		{
		   user2.yspeed+=gravity;
		}

		user2.x+=user2.xspeed;
		user2.y+=user2.yspeed;
		user2.reload+=-1;
	}

	//Check End Game Conditions
	if (user.visible==false || (remote.visible==false && status.prdam!=-1) || (user2.visible==false && status.p2dam!=-1) || status.targets==10)
		status.end++;

	//Show gameover stats
	if (status.end>=90)
		CurrentLoop=DrawStats;



	if (status.asteroids>=0)
	{
		int vis=false;
		//ASTEROIDS
		for (i=0; i<50; i++)
		{
			if (!asteroid[i].visible)
				continue;
			else
				vis=true;

			asteroid[i].x+=asteroid[i].xspeed;
			asteroid[i].y+=asteroid[i].yspeed;
			asteroid[i].yspeed+=gravity;

			if (asteroid[i].y<-(height/2-75))
			{
				asteroid[i].y=-(height/2-74);
				asteroid[i].yspeed=-asteroid[i].yspeed-1;
			}

			#ifndef FULLSCREEN
				if (asteroid[i].x<-width/2)
			#else
				if (asteroid[i].x<(-width/2)*1.778)
			#endif
			{
				asteroid[i].x=asteroid[i].y;
				asteroid[i].y=height/2;
				asteroid[i].yspeed=0;
			}

			#ifndef FULLSCREEN
				if (asteroid[i].x>width/2)
			#else
				if (asteroid[i].x>(width/2)*1.778)
			#endif
			{
				asteroid[i].x=-asteroid[i].y;
				asteroid[i].y=height/2;
				asteroid[i].yspeed=0;
			}
		}
		if (!vis && status.targets==-1) //What?? Why am I checking targets here?
		{
			if (aN<50)
				aN++;
			for (i=0; i<aN; i++)
			{
				rand(); //Randomize
				asteroid[aC].x=width*rand()/RAND_MAX-(width/2);
				asteroid[aC].y=250;
				asteroid[aC].xspeed=(8*((float)rand()/RAND_MAX))-4;
				asteroid[aC].yspeed=0;
				asteroid[aC].visible=true;
				asteroid[aC].size=2;

				aC++;
				if (aC>=50)
					aC=0;
			}
		}
	}

     //EFFECTS-
     for (i=0; i<500; i++)
     {
         if (!effect[i].visible)
            continue;

         effect[i].x+=effect[i].xspeed;
         effect[i].y+=effect[i].yspeed;
         effect[i].life+=-1;

         if (effect[i].life<0)
            effect[i].visible=false;

         effect[i].yspeed+=gravity;

         if (effect[i].y<-(height/2-75))
         {
            effect[i].y=-(height/2-76);
            effect[i].yspeed=0;
         }
     }

	return;
}

void TargetStep()
{
	int i;
	for (i=0; i<10; i++)
	{
		if (BulletXTarget(bt1[i], target[i]))
		{
//			bt1[i].visible=false;
			DestroyBullet(i);
			target[i].visible=false;

			int j;
			for (j=0; j<50; j++)
			{
				effect[effectC].visible=true;
				effect[effectC].life=(short)(75+(75*((float)rand()/RAND_MAX)));
				effect[effectC].r=1.0*((float)rand()/RAND_MAX);
				effect[effectC].g=effect[effectC].r/3;
				effect[effectC].b=0;
				effect[effectC].x=target[j].x+(rand()%8)-4;
				effect[effectC].y=target[j].y+(rand()%8)-4;
				effect[effectC].xspeed=-cos(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
				effect[effectC].xspeed+=bt1[i].xspeed;
				effect[effectC].yspeed=-sin(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
				effect[effectC].yspeed+=bt1[i].yspeed;

				effectC++;
				if (effectC>500)
					effectC=0;
			}
		}
	}

	return;
}

void AsteroidStep()
{

	return;
}

void Collisions()
{
     if (user.visible && ShipXGround(user))
     {
        if (fabs(user.yspeed)<3 && fabs(user.xspeed)<2 && user.dir>75 && user.dir<105)
        {
           user.land=true;
        }
        else
        {
            user.visible=false;
            Crash(user);
        }
     }
     else
     {
         user.land=false;
     }
     
     if (user2.visible && ShipXGround(user2))
     {
        if (fabs(user2.yspeed)<3 && fabs(user2.xspeed)<2 && user2.dir>75 && user2.dir<105)
        {
           user2.land=true;
        }
        else
        {
            user2.visible=false;
            Crash(user2);
        }
     }
     else
     {
         user2.land=false;
     }
     
     int i;
     for (i=0; i<25; i++)
     {
         if (bt1[i].visible)
         {
            if (user.visible && Bullet1XShip(bt1[i], user))
            {
				#ifndef NONETWORK
					//Indicate that the player has been hit
					ndam=1;
				#endif

		DestroyBullet(i);
		Hit(bt1[i]);
		user.health+=-25;
		status.p1dam+=1;
		if (user.health<=0)
		{
			user.visible=false;
			Crash(user);
		}
	}
		if (user2.visible && Bullet1XShip(bt1[i], user2))
		{
			DestroyBullet(i);
			Hit(bt1[i]);
			user2.health+=-25;
			status.p2dam+=1;
			if (user2.health<=0)
			{
				user2.visible=false;
				Crash(user2);
			}
		}
			if (remote.visible && Bullet1XShip(bt1[i], remote))
			{
				DestroyBullet(i);
				Hit(bt1[i]);
				remote.health+=-25;
				status.prdam+=1;
				if (remote.health<=0)
				{
					remote.visible=false;
					Crash(remote);
				}
			}

            if (status.targets>=0)
            {
            	int j;
               for (j=0; j<10; j++)
               {
                   if (BulletXTarget(bt1[i], target[j]))
                   {
			DestroyBullet(i);
                      target[j].visible=false;
                      
                      status.targets++;
                      
                      int k;
                      for (k=0; k<50; k++)
                      {
                      effect[effectC].visible=true;
                      effect[effectC].life=(short)(75+(75*((float)rand()/RAND_MAX)));
                      effect[effectC].r=1.0*((float)rand()/RAND_MAX);
                      effect[effectC].g=effect[effectC].r/3;
                      effect[effectC].b=0;
                      effect[effectC].x=target[j].x+(rand()%8)-4;
                      effect[effectC].y=target[j].y+(rand()%8)-4;
                      effect[effectC].xspeed=-cos(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
                      effect[effectC].xspeed+=bt1[i].xspeed;
                      effect[effectC].yspeed=-sin(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
                      effect[effectC].yspeed+=bt1[i].yspeed;
         
                      effectC++;
                      if (effectC>500)
                         effectC=0;
                      }
                   }
               }
            }
            if (status.asteroids>=0)
            {
    		int j;
               for (j=0; j<50; j++)
               {
                   if (asteroid[j].visible && BulletXAsteroid(bt1[i], asteroid[j]))
                   {
			DestroyBullet(i);
                      asteroid[j].size+=-1;
                      
                      status.asteroids++;
                      
                      if (asteroid[j].size<1)
                      {
                         asteroid[j].visible=false;
                         int i;
                         for (i=0; i<25; i++)
                         {
                             effect[effectC].visible=true;
                             effect[effectC].life=(short)(75+(75*((float)rand()/RAND_MAX)));
                             effect[effectC].r=1.0*((float)rand()/RAND_MAX);
                             effect[effectC].g=effect[effectC].r;
                             effect[effectC].b=effect[effectC].r;
                             effect[effectC].x=asteroid[j].x+(rand()%8)-4;
                             effect[effectC].y=asteroid[j].y+(rand()%8)-4;
         
                             effect[effectC].xspeed=-cos(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
                             effect[effectC].xspeed+=asteroid[j].xspeed;
                             effect[effectC].yspeed=-sin(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
                             effect[effectC].yspeed+=asteroid[j].yspeed;
         
                             effectC++;
                             if (effectC>500)
                                effectC=0;
                         }
                      }
                      else
                      {
                         asteroid[aC].visible=true;
                         asteroid[aC].x=asteroid[j].x;
                         asteroid[aC].y=asteroid[j].y;
                         asteroid[aC].yspeed=asteroid[j].yspeed;
                         asteroid[aC].xspeed=asteroid[j].xspeed+2.5;
                         asteroid[aC].size=1;
                         
                         aC++;
                         if (aC>=50)
                            aC=0;
                         
                         asteroid[aC].visible=true;
                         asteroid[aC].x=asteroid[j].x;
                         asteroid[aC].y=asteroid[j].y;
                         asteroid[aC].yspeed=asteroid[j].yspeed;
                         asteroid[aC].xspeed=asteroid[j].xspeed-2.5;
                         asteroid[aC].size=1;
                         
                         aC++;
                         if (aC>=50)
                            aC=0;
                         
                         asteroid[j].xspeed=bt1[i].xspeed;
                         asteroid[j].yspeed=bt1[i].yspeed;
                         
                         int i;
                         for (i=0; i<10; i++)
                         {
                             effect[effectC].visible=true;
                             effect[effectC].life=(short)(75+(75*((float)rand()/RAND_MAX)));
                             effect[effectC].r=1.0*((float)rand()/RAND_MAX);
                             effect[effectC].g=effect[effectC].r;
                             effect[effectC].b=effect[effectC].r;
                             effect[effectC].x=asteroid[j].x+(rand()%8)-4;
                             effect[effectC].y=asteroid[j].y+(rand()%8)-4;

                             effect[effectC].xspeed=-cos(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
                             effect[effectC].xspeed+=asteroid[j].xspeed;
                             effect[effectC].yspeed=-sin(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
                             effect[effectC].yspeed+=asteroid[j].yspeed;
         
                             effectC++;
                             if (effectC>500)
                                effectC=0;
                         }
                      }
                   }
                   
               }
            }
         }
    }
    if (status.asteroids>=0)
    {
     	int j;
        for (j=0; j<50; j++)
        {
            if (asteroid[j].visible && user.visible)
            if (AsteroidXShip(asteroid[j], user))
            {
                      user.health+=-20;
			status.p1dam+=1;
                      if (user.health<=0)
                      {
                         user.visible=false;
                         Crash(user);
                      }
                      user.xspeed=asteroid[j].xspeed;
                      user.yspeed=asteroid[j].yspeed;
                      if (asteroid[j].size==2)
                      {
                         asteroid[aC].visible=true;
                         asteroid[aC].x=asteroid[j].x+25;
                         asteroid[aC].y=asteroid[j].y;
                         asteroid[aC].yspeed=asteroid[j].yspeed;
                         asteroid[aC].xspeed=asteroid[j].xspeed+2.5;
                         asteroid[aC].size=1;
                         
                         aC++;
                         if (aC>=50)
                            aC=0;
                         
                         asteroid[aC].visible=true;
                         asteroid[aC].x=asteroid[j].x-25;
                         asteroid[aC].y=asteroid[j].y;
                         asteroid[aC].yspeed=asteroid[j].yspeed;
                         asteroid[aC].xspeed=asteroid[j].xspeed-2.5;
                         asteroid[aC].size=1;
                         
                         aC++;
                         if (aC>=50)
                            aC=0;
                      }
     
                      asteroid[j].size=0;
                      asteroid[j].visible=false;
                      
                      int i;
                      for (i=0; i<25; i++)
                      {
                          effect[effectC].visible=true;
                          effect[effectC].life=(short)(75+(75*((float)rand()/RAND_MAX)));
                          effect[effectC].r=1.0*((float)rand()/RAND_MAX);
                          effect[effectC].g=effect[effectC].r;
                          effect[effectC].b=effect[effectC].r;
                          effect[effectC].x=asteroid[j].x+(rand()%8)-4;
                          effect[effectC].y=asteroid[j].y+(rand()%8)-4;
         
                          effect[effectC].xspeed=-cos(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
                          effect[effectC].xspeed+=asteroid[j].xspeed;
                          effect[effectC].yspeed=-sin(((float)rand()/RAND_MAX)*(2*PI))*((float)rand()/RAND_MAX)*5;
                          effect[effectC].yspeed+=asteroid[j].yspeed;
         
                          effectC++;
                          if (effectC>500)
                             effectC=0;
                      }
            }
        }
     }
}

int ShipXGround(struct craft craft)
{
     float  dr1,dr2,dr3;
     double d1,d2,d3,
            x1,x2,x3,
            y1,y2,y3;
            
     d1=15;
     d2=14.142135623730950488016887242097;
     d3=14.142135623730950488016887242097;
     
     dr1=0;
     dr2=135;
     dr3=215;
      
     x1=(craft.x/w2)+(cos((craft.dir+dr1)*(PI/180)))*(d1/w2);
     x2=(craft.x/w2)+(cos((craft.dir+dr2)*(PI/180)))*(d2/w2);
     x3=(craft.x/w2)+(cos((craft.dir+dr3)*(PI/180)))*(d3/w2);
     y1=(craft.y/h2)+(sin((craft.dir+dr1)*(PI/180)))*(d1/h2);
     y2=(craft.y/h2)+(sin((craft.dir+dr2)*(PI/180)))*(d2/h2);
     y3=(craft.y/h2)+(sin((craft.dir+dr3)*(PI/180)))*(d3/h2);
     
     if (XLines(x1,y1,x2,y2,width/w2,-(height/2-75)/h2,-width/w2,-(height/2-75)/h2))
        return true;
        
     if (XLines(x2,y2,x3,y3,width/w2,-(height/2-75)/h2,-width/w2,-(height/2-75)/h2))
        return true;
        
     if (XLines(x3,y3,x1,y1,width/w2,-(height/2-75)/h2,-width/w2,-(height/2-75)/h2))
        return true;
        
     return false;
}

int Bullet1XShip(struct bulletType1 b, struct craft craft)
{
     if (b.active>0)
        return false;
     
     float  dr1,dr2,dr3;
     double d1,d2,d3,
            x1,x2,x3,
            y1,y2,y3;
            
     d1=15;
     d2=14.142135623730950488016887242097;
     d3=14.142135623730950488016887242097;
     
     dr1=0;
     dr2=135;
     dr3=215;
      
     x1=(craft.x/w2)+(cos((craft.dir+dr1)*(PI/180)))*(d1/w2);
     x2=(craft.x/w2)+(cos((craft.dir+dr2)*(PI/180)))*(d2/w2);
     x3=(craft.x/w2)+(cos((craft.dir+dr3)*(PI/180)))*(d3/w2);
     y1=(craft.y/h2)+(sin((craft.dir+dr1)*(PI/180)))*(d1/h2);
     y2=(craft.y/h2)+(sin((craft.dir+dr2)*(PI/180)))*(d2/h2);
     y3=(craft.y/h2)+(sin((craft.dir+dr3)*(PI/180)))*(d3/h2);
     
     if (XLines(x1,y1,x2,y2, (b.x-b.xspeed)/w2, (b.y-b.yspeed)/h2, (b.x+b.xspeed)/w2, (b.y+b.yspeed)/h2))
        return true;
        
     if (XLines(x2,y2,x3,y3, (b.x-b.xspeed)/w2, (b.y-b.yspeed)/h2, (b.x+b.xspeed)/w2, (b.y+b.yspeed)/h2))
        return true;
        
     if (XLines(x3,y3,x1,y1, (b.x-b.xspeed)/w2, (b.y-b.yspeed)/h2, (b.x+b.xspeed)/w2, (b.y+b.yspeed)/h2))
        return true;
        
     return false;
}

int AsteroidXShip(struct asteroids a, struct craft craft)
{    
     float xa1,xa2,
           ya1,ya2,
           xc1,xc2,
           yc1,yc2;
     
     xc1=craft.x-15;
     xc2=craft.x+15;
     yc1=craft.y-15;
     yc2=craft.y+15;
            
     xa1=a.x-(10*a.size);
     xa2=a.x+(10*a.size);
     ya1=a.y-(10*a.size);
     ya2=a.y+(10*a.size);
     
     if (xa2 < xc1)
        return false;
     if (xa1 > xc2)
        return false;

     if (ya2 < yc1)
        return false;
     if (ya1 > yc2)
        return false;
     
     return true;
}

int BulletXTarget(struct bulletType1 b, struct targets a)
{
     if (b.active>0)
        return false;
     
     if (!a.visible)
        return false;
        
     float dist, x,y;
     
     x = fabs(a.x-b.x);
     y = fabs(a.y-b.y);
     
     dist = sqrt((x*x)+(y*y));
     
     if (dist<25)
        return true;
     else
        return false; 
}

int BulletXAsteroid(struct bulletType1 b, struct asteroids a)
{
	if (b.active>0)
		return false;

	float  x1,x2,
			y1,y2;

	x1=a.x+(10*a.size);
	x2=a.x-(10*a.size);
	y1=a.y+(10*a.size);
	y2=a.y-(10*a.size);

	if (XLines(x1,y1,x1,y2, b.x-b.xspeed, b.y-b.yspeed, b.x+b.xspeed, b.y+b.yspeed))
		return true;

	if (XLines(x1,y2,x2,y2, b.x-b.xspeed, b.y-b.yspeed, b.x+b.xspeed, b.y+b.yspeed))
		return true;

	if (XLines(x2,y2,x2,y1, b.x-b.xspeed, b.y-b.yspeed, b.x+b.xspeed, b.y+b.yspeed))
		return true;

	if (XLines(x2,y1,x1,y1, b.x-b.xspeed, b.y-b.yspeed, b.x+b.xspeed, b.y+b.yspeed))
		return true;

	return false;
}


void Keys()
{
	if (key[VK_UP])
	{
		user.xspeed+=cos(user.dir*(PI/180))*thrust;
		user.yspeed+=sin(user.dir*(PI/180))*thrust;

		Thrust(user);
	}
	if (key[VK_LEFT])
	{
		user.dir+=5;
		if (user.dir>360)
			user.dir+=-360;
	}
	if (key[VK_RIGHT])
	{
		user.dir+=-5;
		if (user.dir<0)
			user.dir+=360; 
	}
	if (key[VK_SPACE])
	{
		if (user.reload<=0)
		{
			#ifndef NONETWORK
				//Indicate that the player has fired
				nfire=1;
			#endif
			Fire(user);
			status.p1shots+=1;
			user.reload=30;
		}
	}
	if (key[VK_W])
	{
		user2.xspeed+=cos(user2.dir*(PI/180))*thrust;
		user2.yspeed+=sin(user2.dir*(PI/180))*thrust;

		Thrust(user2);
	}
	if (key[VK_A])
	{
	user2.dir+=5;
	if (user2.dir>360)
		user2.dir+=-360;
	}
	if (key[VK_D])
	{
		user2.dir+=-5;
		if (user2.dir<0)
			user2.dir+=360;
	}
	if (key[VK_E])
	{
		if (user2.reload<=0)
		{
			Fire(user2);
			status.p2shots+=1;
			user2.reload=30;
		}
	}
	if (key[VK_ESC])
		CurrentLoop=MenuInit;

	return;
}

void PlayOnce(int s)
{
	#ifdef AUDIO

		int i;
		alGetSourcei(source[s], AL_SAMPLE_OFFSET, &i);
		if (i==0)
			alSourcePlay(source[s]);

	#endif

	return;
}

void PlaySounds()
{
	#ifdef AUDIO

		int i;
		for (i=0;i<25;i++)
		{
			if (bt1[i].visible==false || bt1[i].active>0)
				continue;

			float u1d=sqrt(((user.x-bt1[i].x)*(user.x-bt1[i].x))+((user.y-bt1[i].y)*(user.y-bt1[i].y)));
			float u2d=sqrt(((user2.x-bt1[i].x)*(user2.x-bt1[i].x))+((user2.y-bt1[i].y)*(user2.y-bt1[i].y)));

			if ((user.visible && u1d<=u2d) || !user2.visible)
			{
				listenerPos[0]=user.x/16;
				listenerPos[1]=user.y/16;
				listenerPos[2]=0;
				sourcePos[0]=bt1[i].x/16;
				sourcePos[1]=bt1[i].y/16;
				sourcePos[2]=0;
				listenerVel[0]=user.xspeed*16;
				listenerVel[1]=user.yspeed*16;
				listenerOri[0]=(user.x/16)+(user.xspeed/16);
				listenerOri[1]=(user.y/16)+(user.yspeed/16);
				sourceOri[0]=(bt1[i].x/16)+(bt1[i].xspeed/16);
				sourceOri[1]=(bt1[i].y/16)+(bt1[i].yspeed/16);
				sourceVel[0]=bt1[i].xspeed*16;
				sourceVel[1]=bt1[i].yspeed*16;
			}
			else 
			{
				listenerPos[0]=user2.x/16;
				listenerPos[1]=user2.y/16;
				listenerPos[2]=0;
				sourcePos[0]=bt1[i].x/16;
				sourcePos[1]=bt1[i].y/16;
				sourcePos[2]=0;
				listenerVel[0]=user.xspeed*16;
				listenerVel[1]=user.yspeed*16;
				sourceVel[0]=bt1[i].xspeed*16;
				sourceVel[1]=bt1[i].yspeed*16;
				listenerOri[0]=(user2.x/16)+(user2.xspeed/16);
				listenerOri[1]=(user2.y/16)+(user2.yspeed/16);
				sourceOri[0]=(bt1[i].x/16)+(bt1[i].xspeed/16);
				sourceOri[1]=(bt1[i].y/16)+(bt1[i].yspeed/16);
			}

				alSourcefv (source[i], AL_POSITION, sourcePos);
				alSourcefv (source[i], AL_VELOCITY, sourceVel);
				alSourcefv (source[i], AL_DIRECTION, sourceOri);

				alListenerfv(AL_POSITION,listenerPos);
				alListenerfv(AL_VELOCITY,listenerVel);
				alListenerfv(AL_ORIENTATION,listenerOri);

				if (bt1[i].active==0)
					PlayOnce(i);
		}
		if ((KeyState(VK_UP)==2 && user.visible) || (KeyState(VK_W)==2 && user2.visible))
		{
			alSourcefv(source[27], AL_POSITION, listenerPos);
			alSourcefv(source[27], AL_VELOCITY, listenerVel);
			alSourcefv(source[27], AL_DIRECTION, listenerOri);

			PlayOnce(27);
		}
		else
		{
			alSourceStop(source[27]);
		}

	#endif
	return;
}

inline void ShowText(unsigned char *str)
{
	int i;
	for (i=0;i<strlen(str);i++)
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,str[i]);

	return;
}

void MenuInit()
{
	if (KeyState(VK_RETURN) || KeyState(VK_SPACE))
		return;

	menu.option=0;
	menu.help = 0;
	menu.r=0;
	menu.g=0;
	menu.b=0;
	ResetStats();

	CurrentLoop=MenuMain;

	return;
}

void MenuMain()
{
	if (menu.r<1 || menu.g<1 || menu.b<1)
	{
		menu.r+=0.1;
		menu.g+=0.02;
		menu.b+=0.01;
	}
     
	glLoadIdentity();
	glTranslated((-width)/w2,(-(height/2)/h2)+2.0,-3);
	glTranslated(1,0,1);
	glScaled(0.00075,0.00075,0.1);
	ShowText("Untitled One");

	glColor3f(menu.b, menu.r, menu.g);

	if (menu.rot<90 || menu.rot>270)
	{
		glLoadIdentity();
		glTranslated((-width)/w2,(-(height/2)/h2)+0.75,-3);
		glRotated(menu.rot,1,0,0);
		glTranslated(1.8,0,0.5);
		glScaled(0.00075,0.00075,0.1);
		ShowText("Classic");
	}

	if (menu.rot<210 && menu.rot>30)
	{
		glLoadIdentity();
		glTranslated((-width)/w2,(-(height/2)/h2)+0.75,-3);
		glRotated(menu.rot+240,1,0,0);
		glTranslated(1.8,0,0.5);
		glScaled(0.00075,0.00075,0.1);
		ShowText("Targets");
	}

	if (menu.rot<150 || menu.rot>330)
	{
		glLoadIdentity();
		glTranslated((-width)/w2,(-(height/2)/h2)+0.75,-3);
		glRotated(menu.rot+300,1,0,0);
		glTranslated(1.8,0,0.5);
		glScaled(0.00075,0.00075,0.1);
		ShowText("Asteroids");
	}

	if (menu.rot<30 || menu.rot>210)
	{
		glLoadIdentity();
		glTranslated((-width)/w2,(-(height/2)/h2)+0.75,-3);
		glRotated(menu.rot+60,1,0,0);
		glTranslated(1.8,0,0.5);
		glScaled(0.00075,0.00075,0.1);
		ShowText("Exit");
	}

	if (menu.rot<330 && menu.rot>150)
	{
		glLoadIdentity();
		glTranslated((-width)/w2,(-(height/2)/h2)+0.75,-3);
		glRotated(menu.rot+120,1,0,0);
		glTranslated(1.8,0,0.5);
		glScaled(0.00075,0.00075,0.1);
		ShowText("Help");
	}

	if (menu.rot<270 && menu.rot>90)
	{
		glLoadIdentity();
		glTranslated((-width)/w2,(-(height/2)/h2)+0.75,-3);
		glRotated(menu.rot+180,1,0,0);
		glTranslated(1.8,0,0.5);
		glScaled(0.00075,0.00075,0.1);
		#ifndef NONETWORK
			ShowText("Network");
		#else
			glColor3f(0.1, 0.1, 0.1);
			ShowText("Disabled");
			glColor3f(menu.b, menu.r, menu.g);
		#endif
	}

	glLoadIdentity();
	glTranslated((-width)/w2,(-(height/2)/h2)+0.75+(10/w2),-3);
	glRotated(((menu.rot+320)%60)+160,1,0,0);
	glTranslated(1.8+(-10/w2),0,0.5);
	glBegin(GL_TRIANGLES);
		glColor3f (1, 0.5, 0);   glVertex3f (15/w2, 0/h2, -1);
		glColor3f (1, 0.5, 0);   glVertex3f (-10/w2, -10/h2, -1);
		glColor3f (1, 0.5, 0);   glVertex3f (-10/w2, 10/h2, -1);
	glEnd();

	glLoadIdentity();
	glTranslated(0.22,-1.3,-3);
	glScaled(0.0005,0.0005,1);
	ShowText("Use arrow keys to scroll");
	glLoadIdentity();
	glTranslated(0.22,-1.4,-3);
	glScaled(0.0005,0.0005,1);
	ShowText("Press 'Enter' to select");

	if (KeyState(VK_UP))
		menu.rot+=-5;

	if (KeyState(VK_DOWN))
		menu.rot+=5;

	if (menu.rot>360)
		menu.rot+=-360;

	if (menu.rot<0)
		menu.rot+=360;

	if (KeyState(VK_RETURN))
	{
		if (menu.rot>280 && menu.rot<320)
			glutLeaveMainLoop();

		if (menu.rot<20 || menu.rot>340)
			CurrentLoop=GameInit;

		if (menu.rot<200 && menu.rot>160)
		{
			#ifndef NONETWORK
				CurrentLoop=NetworkSetup;
			#endif
		}

		if (menu.rot<80 && menu.rot>40)
			CurrentLoop=AsteroidSetup;
		   
		if (menu.rot<140 && menu.rot>100)
			CurrentLoop=TargetSetup;
		   
		if (menu.rot<260 && menu.rot>220)
			CurrentLoop=HelpSetup;
	}

	glutSwapBuffers();

	return;
}

void DrawStats()
{
	unsigned char text[50];

	glLoadIdentity();
	glColor3f(0.85f, 0.5f, 0);
	glTranslated(-1.5,1.4,-3);
	glScaled(0.0005,0.0005,1);
	if (status.timer>0)
	{
		ShowText("Round Time: ");
		sprintf(text,"%5.2f",(float)(status.timer/fps));
		ShowText(text);
		ShowText(" Seconds");
	}

	glLoadIdentity();
	glTranslated(-1.5,1.1,-3);
	glScaled(0.0005,0.0005,1);
	if (status.p1shots>=0)
	{
		ShowText("Player1 Shots Fired: ");
		sprintf(text,"%d",status.p1shots);
		ShowText(text);
	}

	glLoadIdentity();
	glTranslated(-1.5,1,-3);
	glScaled(0.0005,0.0005,1);
	if (status.p1dam>=0)
	{
		ShowText("Player1 Hits Taken: ");
		sprintf(text,"%d",status.p1dam);
		ShowText(text);
	}
	glLoadIdentity();
	glTranslated(0,1.1,-3);
	glScaled(0.0005,0.0005,1);
	if (status.p2shots>=0)
	{
		ShowText("Player2 Shots Fired: ");
		sprintf(text,"%d",status.p2shots);
		ShowText(text);
	}
	else if (status.prshots>=0)
	{
		ShowText("Remote Player Shots Fired: ");
		sprintf(text,"%d",status.prshots);
		ShowText(text);
	}

	glLoadIdentity();
	glTranslated(0,1,-3);
	glScaled(0.0005,0.0005,1);
	if (status.p2dam>=0)
	{
		ShowText("Player2 Hits Taken: ");
		sprintf(text,"%d",status.p2dam);
		ShowText(text);
	}
	else if (status.prdam>=0)
	{
		ShowText("Remote Player Hits Taken: ");
		sprintf(text,"%d",status.prdam);
		ShowText(text);
	}

	glLoadIdentity();
	glTranslated(0,1,-3);
	glScaled(0.0005,0.0005,1);
	if (status.targets>=0)
	{
		ShowText("Targets Destroyed: ");
		sprintf(text,"%d",status.targets);
		ShowText(text);
	}

	glLoadIdentity();
	glTranslated(0,1,-3);
	glScaled(0.0005,0.0005,1);
	if (status.asteroids>=0)
	{
		ShowText("Asteroids Hit: ");
		sprintf(text,"%d",status.asteroids);
		ShowText(text);
	}

	glLoadIdentity();
	glTranslated(-1,0,-3);
	glScaled(0.001,0.001,1);
	if (status.asteroids==-1 && status.targets==-1)
	{
		if (status.prdam==-1)
		{
			if (user.visible==true)
				ShowText("Player 1 Wins!");
			else
			if (user2.visible==true)
				ShowText("Player 2 Wins!");
			else
				ShowText("Draw.");

			if (KeyState(VK_RETURN))
				CurrentLoop=GameInit;
		}
		else
		{
			if (user.visible==true)
				ShowText("You Win!");
			else
			if (remote.visible==true)
				ShowText("You Lose!");
			else
				ShowText("Draw.");

			if (KeyState(VK_RETURN))
				CurrentLoop=NetworkSetup;
		}
	}
	if (status.targets>=0)
	{
		if (user.visible==true)
			ShowText("Success!");
		else
			ShowText("Failure!");	
		
		if (KeyState(VK_RETURN))
			CurrentLoop=TargetSetup;
	}
	if (status.asteroids>=0)
	{
		if (status.asteroids==0)
			ShowText("Failure!");
		if (status.asteroids>0 && status.asteroids<9)
			ShowText("Terrible!");
		if (status.asteroids>=9 && status.asteroids<15)
			ShowText("Average");
		if (status.asteroids>=15 && status.asteroids<21)
			ShowText("Good!");
		if (status.asteroids>=21 && status.asteroids<32)
			ShowText("Exellent!");
		if (status.asteroids>=32)
			ShowText("Amazing!");
			
		if (KeyState(VK_RETURN))
			CurrentLoop=AsteroidSetup;
	}

	glLoadIdentity();
	glTranslated(0.22,-1.3,-3);
	glScaled(0.0005,0.0005,1);
	ShowText("Press 'Enter' to Restart");
	glLoadIdentity();
	glTranslated(0.22,-1.4,-3);
	glScaled(0.0005,0.0005,1);
	ShowText("Press 'Esc' to Quit to Menu");

	glutSwapBuffers();

	return;
}

void DrawAll()
{
	#ifdef INSTRUMENT
		int timea,timeb,timedelta;
		timea=glutGet(GLUT_ELAPSED_TIME);
	#endif

	DrawShip(user);
	DrawShip(user2);
	DrawAsteroids();
	DrawShip(remote);

	#ifdef INSTRUMENT
		timeb=glutGet(GLUT_ELAPSED_TIME);
		printf("*%d* : ",timeb-timea);
	#endif

	DrawBullets();
	DrawGround();
	DrawEffects();

	DrawTargets();

	#ifdef INSTRUMENT
		timea=glutGet(GLUT_ELAPSED_TIME);
		printf("*%d* : ",timea-timeb);
	#endif

	glutSwapBuffers();

	return;
}

//Here we compile the ship model into a display list in order to increase runtime speed
GLuint BuildShip(struct craft craft)
{
	//Oh shit, this is some bad code....
	//(checking craft.list but returning a value which we assume will be stored in craft.list)
	if (glIsList(craft.list))
		glDeleteLists(craft.list,1);

	GLuint list = glGenLists(1);

	glNewList(list, GL_COMPILE);
		glBegin (GL_TRIANGLES);
		glColor3f (craft.r1, craft.g1, craft.b1);   glVertex3f (15/w2, 0/h2, 0);
		glColor3f (craft.r2, craft.g2, craft.b2);   glVertex3f (-10/w2, -10/h2, 10/h2);
		glColor3f (craft.r3, craft.g3, craft.b3);   glVertex3f (-10/w2, 10/h2, 10/h2);

		glColor3f (craft.r1, craft.g1, craft.b1);   glVertex3f (15/w2, 0/h2, 0);
		glColor3f (craft.r2, craft.g2, craft.b2);   glVertex3f (-10/w2, 10/h2, -10/h2);
		glColor3f (craft.r3, craft.g3, craft.b3);   glVertex3f (-10/w2, 10/h2, 10/h2);

		glColor3f (craft.r1, craft.g1, craft.b1);   glVertex3f (15/w2, 0/h2, 0);
		glColor3f (craft.r2, craft.g2, craft.b2);   glVertex3f (-10/w2, 10/h2, -10/h2);
		glColor3f (craft.r3, craft.g3, craft.b3);   glVertex3f (-10/w2, -10/h2, -10/h2);

		glColor3f (craft.r1, craft.g1, craft.b1);   glVertex3f (15/w2, 0/h2, 0);
		glColor3f (craft.r2, craft.g2, craft.b2);   glVertex3f (-10/w2, -10/h2, 10/h2);
		glColor3f (craft.r3, craft.g3, craft.b3);   glVertex3f (-10/w2, -10/h2, -10/h2);

		glColor3f (1.0f, 0.0f, 0.0f);   glVertex3f (-6/w2, 0/h2, 0);
		glColor3f (craft.r2, craft.g2, craft.b2);   glVertex3f (-10/w2, -10/h2, 10/h2);
		glColor3f (craft.r2, craft.g3, craft.b3);   glVertex3f (-10/w2, 10/h2, 10/h2);

		glColor3f (1.0f, 0.0f, 0.0f);   glVertex3f (-6/w2, 0/h2, 0);
		glColor3f (craft.r2, craft.g2, craft.b2);   glVertex3f (-10/w2, 10/h2, -10/h2);
		glColor3f (craft.r3, craft.g3, craft.b3);   glVertex3f (-10/w2, 10/h2, 10/h2);

		glColor3f (1.0f, 0.0f, 0.0f);   glVertex3f (-6/w2, 0/h2, 0);
		glColor3f (craft.r2, craft.g2, craft.b2);   glVertex3f (-10/w2, 10/h2, -10/h2);
		glColor3f (craft.r3, craft.g3, craft.b3);   glVertex3f (-10/w2, -10/h2, -10/h2);

		glColor3f (1.0f, 0.0f, 0.0f);   glVertex3f (-6/w2, 0/h2, 0);
		glColor3f (craft.r2, craft.g2, craft.b2);   glVertex3f (-10/w2, -10/h2, 10/h2);
		glColor3f (craft.r3, craft.g3, craft.b3);   glVertex3f (-10/w2, -10/h2, -10/h2);
		glEnd ();
	glEndList();

	return list;
}

void DrawShip(struct craft craft)
{
	if (!craft.visible)
		return;

	glLoadIdentity();
	glTranslated(craft.x/w2,craft.y/h2, -1);
	glRotated(craft.dir, 0, 0, 1);
	glRotated(craft.dir, 1, 0, 0);

	glCallList(craft.list);

	#ifndef FULLSCREEN
		if (craft.x>width/2)
	#else
		if (craft.x>(width*1.778)/2)
	#endif
	{
		glLoadIdentity();
		#ifndef FULLSCREEN
			glTranslated((width/2-100)/w2,craft.y/h2, -1);
		#else
			glTranslated(((width/2-100)/w2)*1.778,craft.y/h2, -1);
		#endif
		glRotated(craft.dir, 0, 0, 1);
		glBegin (GL_TRIANGLES);
		glColor3f (craft.r1/3, craft.g1/3, craft.b1/3);   glVertex3f (15/w2, 0/h2, 0);
		glColor3f (craft.r2/3, craft.g2/3, craft.b2/3);   glVertex3f (-10/w2, -10/h2, 0);
		glColor3f (craft.r3/3, craft.g3/3, craft.b3/3);   glVertex3f (-10/w2, 10/h2, 0);
		glEnd();
		glLoadIdentity();
		#ifndef FULLSCREEN
			glTranslated((width/2-80)/w2,craft.y/h2, -1);
		#else
			glTranslated(((width/2-80)/w2)*1.778,craft.y/h2, -1);
		#endif
		glBegin (GL_TRIANGLES);
		glColor3f (1.1f, 0.1f, 0.3f);   glVertex3f (5/w2, 0/h2, 0);
		glColor3f (1.0f, 0.0f, 0.12f);   glVertex3f (0/w2, -5/h2, 0);
		glColor3f (1.0f, 0.0f, 0.24f);   glVertex3f (0/w2, 5/h2, 0);
		glEnd();
	}
	#ifndef FULLSCREEN
		if (craft.x<-(width/2))
	#else
		if (craft.x<-(width*1.778)/2)
	#endif
	{
		glLoadIdentity();
		#ifndef FULLSCREEN
			glTranslated((-(width/2-90)/w2),craft.y/h2, -1);
		#else
			glTranslated((-(width/2-90)/w2)*1.778,craft.y/h2, -1);
		#endif
		glRotated(craft.dir, 0, 0, 1);
		glBegin (GL_TRIANGLES);
		glColor3f (craft.r1/3, craft.g1/3, craft.b1/3);   glVertex3f (15/w2, 0/h2, 0);
		glColor3f (craft.r2/3, craft.g2/3, craft.b2/3);   glVertex3f (-10/w2, -10/h2, 0);
		glColor3f (craft.r3/3, craft.g3/3, craft.b3/3);   glVertex3f (-10/w2, 10/h2, 0);
		glEnd();
		glLoadIdentity();
		#ifndef FULLSCREEN
			glTranslated((-(width/2-70)/w2),craft.y/h2, -1);
		#else
			glTranslated((-(width/2-70)/w2)*1.778,craft.y/h2, -1);
		#endif
		glRotated(180, 0, 0, 1);
		glBegin (GL_TRIANGLES);
		glColor3f (1.1f, 0.1f, 0.3f);   glVertex3f (5/w2, 0/h2, 0);
		glColor3f (1.0f, 0.0f, 0.12f);   glVertex3f (0/w2, -5/h2, 0);
		glColor3f (1.0f, 0.0f, 0.24f);   glVertex3f (0/w2, 5/h2, 0);
		glEnd();
	}
	if (craft.y>height/2)
	{
		glLoadIdentity();
		glTranslated(craft.x/w2,(height/2-100)/h2, -1);
		glRotated(craft.dir, 0, 0, 1);
		glBegin (GL_TRIANGLES);
		glColor3f (craft.r1/3, craft.g1/3, craft.b1/3);   glVertex3f (15/w2, 0/h2, 0);
		glColor3f (craft.r2/3, craft.g2/3, craft.b2/3);   glVertex3f (-10/w2, -10/h2, 0);
		glColor3f (craft.r3/3, craft.g3/3, craft.b3/3);   glVertex3f (-10/w2, 10/h2, 0);
		glEnd();
		glLoadIdentity();
		glTranslated(craft.x/w2,(height/2-80)/h2, -1);
		glRotated(90, 0, 0, 1);
		glBegin (GL_TRIANGLES);
		glColor3f (1.1f, 0.1f, 0.3f);   glVertex3f (5/w2, 0/h2, 0);
		glColor3f (1.0f, 0.0f, 0.12f);   glVertex3f (0/w2, -5/h2, 0);
		glColor3f (1.0f, 0.0f, 0.24f);   glVertex3f (0/w2, 5/h2, 0);
		glEnd();
	}

	return;
}

void DrawBullets()
{
	int i;
	for (i=0; i<25; i++)
	{
		if (!bt1[i].visible)
			continue;

		glLoadIdentity();
		glTranslated(bt1[i].x/w2,bt1[i].y/h2, -1);
		glRotated(bt1[i].dir, 0, 0, 1);
		glRotated(bt1[i].spin, -1, 0, 0);
		glBegin (GL_TRIANGLES);
		glColor3f (1.0f, 0.3f, 0.3f);   glVertex3f (5/w2, 0/h2, 0);
		glColor3f (0.4f, 0.0f, 0.0f);   glVertex3f (-5/w2, -2/h2, 0.01);
		glColor3f (0.8f, 0.0f, 0.0f);   glVertex3f (-5/w2, 2/h2, -0.01);
		glEnd();

		bt1[i].spin+=5;
	}

	return;
}

void BuildTarget()
{
	target_list = glGenLists(1);

	glNewList(target_list, GL_COMPILE);
		glBegin(GL_POLYGON);
		glColor3f (1, 0.4, 0);   glVertex3f (0/w2, 20/h2, 0/h2);
		glColor3f (1, 0, 0);   glVertex3f (20/w2, 0/h2, 0/h2);
		glColor3f (1, 0, 0);   glVertex3f (10/w2, -20/h2, 0/h2);
		glColor3f (1, 0, 0);   glVertex3f (-10/w2, -20/h2, 0/h2);
		glColor3f (1, 0, 0);   glVertex3f (-20/w2, 0/h2, 0/h2);
		glEnd();

		glBegin(GL_POLYGON);
		glColor3f (1, 1, 1);   glVertex3f (0/w2, 15/h2, 0/h2);
		glColor3f (1, 0.6, 1);   glVertex3f (15/w2, 0/h2, 0/h2);
		glColor3f (1, 1, 1);   glVertex3f (7.5/w2, -15/h2, 0/h2);
		glColor3f (1, 1, 1);   glVertex3f (-7.5/w2, -15/h2, 0/h2);
		glColor3f (1, 1, 1);   glVertex3f (-15/w2, 0/h2, 0/h2);
		glEnd();

		glBegin(GL_POLYGON);
		glColor3f (1, 0, 0);   glVertex3f (0/w2, 10/h2, 0/h2);
		glColor3f (1, 0, 0);   glVertex3f (10/w2, 0/h2, 0/h2);
		glColor3f (1, 0.4, 0);   glVertex3f (5/w2, -10/h2, 0/h2);
		glColor3f (1, 0, 0);   glVertex3f (-5/w2, -10/h2, 0/h2);
		glColor3f (1, 0, 0);   glVertex3f (-10/w2, 0/h2, 0/h2);
		glEnd();

		glBegin(GL_POLYGON);
		glColor3f (1, 1, 1);   glVertex3f (0/w2, 5/h2, 0/h2);
		glColor3f (1, 1, 1);   glVertex3f (5/w2, 0/h2, 0/h2);
		glColor3f (1, 1, 1);   glVertex3f (2.5/w2, -5/h2, 0/h2);
		glColor3f (1, 0.6, 1);   glVertex3f (-2.5/w2, -5/h2, 0/h2);
		glColor3f (1, 1, 1);   glVertex3f (-5/w2, 0/h2, 0/h2);
		glEnd();
	glEndList();
}

void DrawTargets()
{
	glDisable(GL_DEPTH_TEST);
	int i;
	for (i=0; i<10; i++)
	{
		if (!target[i].visible)
			continue;

		glLoadIdentity();
		glTranslated(target[i].x/w2,target[i].y/h2, -1);
		glCallList(target_list);
	}
	glEnable(GL_DEPTH_TEST);

	return;
}


void DrawAsteroids()
{
	int i;
     for (i=0; i<50; i++)
     {
         if (!asteroid[i].visible)
            continue;
         
         if (asteroid[i].size==2)
         {
            glLoadIdentity();
            glTranslated(asteroid[i].x/w2,asteroid[i].y/h2, -1);
            glRotated(asteroid[i].rot, (i%2)+0.2, ((i+1)%2)+0.3, (i+2)%2);
            glBegin (GL_TRIANGLES);
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (20/w2, 0/h2, 20/h2);
            glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (-20/w2, 0/h2, 20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, 20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (20/w2, 0/h2, 20/h2);
            glColor3f (0.5f, 0.5f, 0.5f);   glVertex3f (20/w2, 0/h2, -20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, 20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (20/w2, 0/h2, -20/h2);
            glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (-20/w2, 0/h2, -20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, 20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (20/w2, 0/h2, 20/h2);
            glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (-20/w2, 0/h2, 20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, -20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (20/w2, 0/h2, 20/h2);
            glColor3f (0.5f, 0.5f, 0.5f);   glVertex3f (20/w2, 0/h2, -20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, -20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (20/w2, 0/h2, -20/h2);
            glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (-20/w2, 0/h2, -20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, -20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (-20/w2, 0/h2, 20/h2);
            glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (20/w2, 0/h2, 20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, 20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (-20/w2, 0/h2, 20/h2);
            glColor3f (0.5f, 0.5f, 0.5f);   glVertex3f (-20/w2, 0/h2, -20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, 20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (-20/w2, 0/h2, -20/h2);
            glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (20/w2, 0/h2, -20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, 20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (-20/w2, 0/h2, 20/h2);
            glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (20/w2, 0/h2, 20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, -20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (-20/w2, 0/h2, 20/h2);
            glColor3f (0.5f, 0.5f, 0.5f);   glVertex3f (-20/w2, 0/h2, -20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, -20/h2, 0/h2);
         
            glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (-20/w2, 0/h2, -20/h2);
            glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (-20/w2, 0/h2, -20/h2);
            glColor3f (0.2f, 0.1f, 0.2f);   glVertex3f (0/w2, -20/h2, 0/h2);
            glEnd();
         }
         else
         {
             glLoadIdentity();
             glTranslated(asteroid[i].x/w2,asteroid[i].y/h2, -1);
             glRotated(asteroid[i].rot, (i%2)+0.2, ((i+1)%2)+0.3, (i+2)%2);
             glBegin (GL_QUADS);
             glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (10/w2, 10/h2, 10/h2);
             glColor3f (0.5f, 0.5f, 0.5f);   glVertex3f (10/w2, -10/h2, 10/h2);
             glColor3f (0.8f, 0.8f, 0.8f);   glVertex3f (-10/w2, -10/h2, 10/h2);
             glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (-10/w2, 10/h2, 10/h2);
             
             glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (10/w2, 10/h2, -10/h2);
             glColor3f (0.5f, 0.5f, 0.5f);   glVertex3f (10/w2, -10/h2, -10/h2);
             glColor3f (0.8f, 0.8f, 0.8f);   glVertex3f (-10/w2, -10/h2, -10/h2);
             glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (-10/w2, 10/h2, -10/h2);
             
             glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (-10/w2, 10/h2, 10/h2);
             glColor3f (0.5f, 0.5f, 0.5f);   glVertex3f (-10/w2, -10/h2, 10/h2);
             glColor3f (0.8f, 0.8f, 0.8f);   glVertex3f (-10/w2, -10/h2, -10/h2);
             glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (-10/w2, 10/h2, -10/h2);
             
             glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (10/w2, 10/h2, 10/h2);
             glColor3f (0.5f, 0.5f, 0.5f);   glVertex3f (10/w2, -10/h2, 10/h2);
             glColor3f (0.8f, 0.8f, 0.8f);   glVertex3f (10/w2, -10/h2, -10/h2);
             glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (10/w2, 10/h2, -10/h2);
             
             glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (10/w2, -10/h2, 10/h2);
             glColor3f (0.5f, 0.5f, 0.5f);   glVertex3f (10/w2, -10/h2, -10/h2);
             glColor3f (0.8f, 0.8f, 0.8f);   glVertex3f (-10/w2, -10/h2, -10/h2);
             glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (-10/w2, -10/h2, 10/h2);
             
             glColor3f (0.3f, 0.3f, 0.3f);   glVertex3f (10/w2, 10/h2, 10/h2);
             glColor3f (0.5f, 0.5f, 0.5f);   glVertex3f (10/w2, 10/h2, -10/h2);
             glColor3f (0.8f, 0.8f, 0.8f);   glVertex3f (-10/w2, 10/h2, -10/h2);
             glColor3f (0.4f, 0.4f, 0.4f);   glVertex3f (-10/w2, 10/h2, 10/h2);
             glEnd();
         }
         
         asteroid[i].rot+=5*((i%5)+1);
     }

     return;
}


void DrawGround()
{
	glLoadIdentity();
	glBegin(GL_QUADS);
	glColor3f (0.1f, 0.3f, 0.1f);   glVertex3f ((width)/w2, -(height/2-75)/h2, -3);
	glColor3f (0.4f, 0.7f, 0.0f);   glVertex3f (-(width)/w2, -(height/2-75)/h2, -3);
	glColor3f (0.0f, 0.5f, 0.2f);   glVertex3f (-(width)/w2, -(height/2-75)/h2, 3);
	glColor3f (0.0f, 0.1f, 0.0f);   glVertex3f ((width)/w2, -(height/2-75)/h2, 3);
	glEnd();

	return;
}

void DrawEffects()
{
	int i;
	for (i=0; i<500; i++)
	{
		if (!effect[i].visible)
			continue;

		glLoadIdentity();
		glBegin (GL_LINES);
		glColor3f (effect[i].r, effect[i].g, effect[i].b);   glVertex3f (effect[i].x/w2, effect[i].y/h2, -1);
		glColor3f (effect[i].r, effect[i].g, effect[i].b);   glVertex3f ((effect[i].x-effect[i].xspeed)/w2, (effect[i].y-effect[i].yspeed)/h2, -1);
		glEnd();
	}

	return;
}


void Thrust(struct craft craft)
{
	if (!craft.visible)
		return;
        
	int i;
	for (i=0; i<5; i++)
	{
		effect[effectC].visible=true;
		effect[effectC].life=(short)(10+(5*((float)rand()/RAND_MAX)));
		effect[effectC].r=10.0*((float)rand()/RAND_MAX);
		effect[effectC].g=1.0*((float)rand()/RAND_MAX);
		effect[effectC].b=0.1;
		effect[effectC].x=craft.x+(rand()%8)-4;
		effect[effectC].y=craft.y+(rand()%8)-4;

		effect[effectC].xspeed=-cos(craft.dir*(PI/180))*((float)rand()/RAND_MAX)*7;
		effect[effectC].yspeed=-sin(craft.dir*(PI/180))*((float)rand()/RAND_MAX)*7;

		effectC++;
		if (effectC>500)
			effectC=0;
	}

	return;
}


void Hit(struct bulletType1 bt1)
{
	int i;
     for (i=0; i<5; i++)
     {
         effect[effectC].visible=true;
         effect[effectC].life=(short)(20+(10*((float)rand()/RAND_MAX)));
         effect[effectC].r=10.0*((float)rand()/RAND_MAX);
         effect[effectC].g=1.0*((float)rand()/RAND_MAX);
         effect[effectC].b=0.1;
         effect[effectC].x=bt1.x+(rand()%8)-4;
         effect[effectC].y=bt1.y+(rand()%8)-4;
         
         effect[effectC].xspeed=-bt1.xspeed*((float)rand()/RAND_MAX);
         effect[effectC].yspeed=-bt1.yspeed*((float)rand()/RAND_MAX);
         
         effectC++;
         if (effectC>500)
            effectC=0;
     }

	#ifdef AUDIO
		alSourcefv(source[25], AL_POSITION, listenerPos);
		alSourcefv(source[25], AL_VELOCITY, listenerVel);
		alSourcefv(source[25], AL_DIRECTION, listenerOri);
		PlayOnce(25);
	#endif

	return;
}


int XLines(double x11, double y11, double x12, double y12, double x21, double y21, double x22, double y22)
{     
     double b1,b2,
            a1,a2,
            xi,yi;
     
     if (x11==x12)
         x12+=0.000000001;
     
     if (x21==x22)
         x22+=0.000000001;
         
     if (y11==y12)
         y12+=0.000000001;
     
     if (y21==y22)
         y22+=0.000000001;
         
     b1 = (y22-y21)/(x22-x21);
     b2 = (y12-y11)/(x12-x11);
     if (b1!=b2)
     {
        a1 = y21-b1*x21;
        a2 = y11-b2*x11;
        xi = -(a1-a2)/(b1-b2);
        yi = a1+b1*xi;
         
        if (((xi>=x11 && xi<=x12) || (xi<=x11 && xi>=x12))
           && ((yi>=y11 && yi<=y12) || (yi<=y11 && yi>=y12))
           && ((xi>=x21 && xi<=x22) || (xi<=x21 && xi>=x22))
           && ((yi>=y21 && yi<=y22) || (yi<=y21 && yi>=y22)))
           return true;
        else
           return false;
     }
     else
        return false;
}


void HelpSetup()
{
	if (KeyState(VK_RETURN) || KeyState(VK_SPACE))
		return;

	int i;
	for (i=0; i<50; i++)
		asteroid[i].visible=false;

	status.asteroids=0;
	status.targets=0;

	CurrentLoop=HelpMain;

	return;
}


void HelpMain()
{
	glLoadIdentity();
	glColor3f(1, 0.5f, 0);
	glTranslated(-0.5,-0.4,-6);
	glScaled(0.00075,0.00075,1);

	//Prevent Endgame Screen
	status.end=0;

	glLoadIdentity();
	glColor3f(0.85f, 0.5f, 0);
	glTranslated(-1.5,1.5,-3);
	glScaled(0.0005,0.0005,1);
	if (menu.help>0 && menu.help<=1)
		ShowText("Welcome to Untitled One");
	if (menu.help>2 && menu.help<=3)
		ShowText("Use the arrow keys to maneuver your ship");
	if (menu.help>3 && menu.help<=35)
		ShowText("Press 'up' for thrust");
	if (menu.help>35 && menu.help<=45)
		ShowText("Press 'right' and 'left' to rotate");
	if (menu.help>45 && menu.help<=60)
		ShowText("Press 'space' to fire");
	if (menu.help>60 && menu.help<=70)
		ShowText("Missiles take time to become armed");
	if (menu.help>70 && menu.help<=100)
		ShowText("Only an armed missile leaves a smoke trail");
	if (menu.help>100 && menu.help<=125)
		ShowText("Armed missiles will do damage");
	if (menu.help>125 && menu.help<=150)
		ShowText("Everything is affected by gravity");
	if (menu.help>150 && menu.help<=170)
		ShowText("Be careful not to crash");
	if (menu.help>170 && menu.help<=175)
		ShowText("You don't get a second chance");
	if (menu.help>175 && menu.help<=185)
		ShowText("In Target mode, you can practice shooting");
	if (menu.help>185 && menu.help<=195)
		ShowText("In Asteroids mode, you destroy asteroids");
	if (menu.help>195 && menu.help<=235)
		ShowText("Asteroids break into smaller pieces");
	if (menu.help>235 && menu.help<=285)
		ShowText("Don't get hit by the asteroids");
	if (menu.help>285 && menu.help<=300)
		ShowText("In Classic mode, you battle another player");
	if (menu.help>300 && menu.help<=335)
		ShowText("Player two uses the 'w', 'a', 'd', and 'e' keys");
	if (menu.help>335 && menu.help<=455)
		ShowText("Four hits will destroy your opponent");
	if (menu.help>455 && menu.help<=490)
		ShowText("It is possible to land safely on the ground");
	if (menu.help>490 && menu.help<=535)
		ShowText("Careful, your own missiles can hit you");
	if (menu.help>540 && menu.help<=550)
		ShowText("That's all");
	if (menu.help>550 && menu.help<=560)
		ShowText("Good luck!");

	if (menu.help==0)
	{
		user.x=-200;
		user.y=-205;
		user.xspeed=0;
		user.yspeed=0;
		user.dir=90;
		user.health=100;
		user.visible=true;
		user.r1=0.2; user.g1=0.2; user.b1=0.2;
		user.r2=0.3; user.g2=0.3; user.b2=0.3;
		user.r3=0.9; user.g3=0.9; user.b3=0.9;
		user.range=240;
		user.prime=30;

		user2.visible=false;
		user2.r1=0.3; user2.g1=0.3; user2.b1=0.3;
		user2.r2=0.4; user2.g2=0.4; user2.b2=0.4;
		user2.r3=0.8; user2.g3=0.8; user2.b3=0.8;
		user2.range=240;
		user2.prime=30;

		user.list = BuildShip(user);
		user2.list = BuildShip(user2);

		int i;

		for (i=0; i<50; i++)
			asteroid[i].visible=false;
		for (i=0; i<10; i++)
			target[i].visible=false;
	}
	if (menu.help>3 && menu.help<30)
	{
		user.xspeed+=cos(user.dir*(PI/180))*thrust;
		user.yspeed+=sin(user.dir*(PI/180))*thrust;      
		Thrust(user);
	}
	if (menu.help>35 && menu.help<45)
	{
		user.dir+=-5;
		if (user.dir<0)
			user.dir+=360;
	}
	if (menu.help>45 && menu.help<60)
	{
		user.xspeed+=cos(user.dir*(PI/180))*thrust;
		user.yspeed+=sin(user.dir*(PI/180))*thrust;      
		Thrust(user);

		if (menu.help==50)
			Fire(user);
	}
	if (menu.help>60 && menu.help<=61)
	{
		target[0].x=-100;
		target[0].y=25;
		target[0].visible=true;

		if (!glIsList(target_list))
			BuildTarget();
	}
	if (menu.help>100 && menu.help<=101)
	{
		target[0].x=90;
		target[0].y=110;
		target[0].visible=true;
	}
	if (menu.help>175 && menu.help<=176)
	{
		target[0].x=0;
		target[0].y=0;
		target[0].visible=true;
		target[1].x=75;
		target[1].y=-25;
		target[1].visible=true;
		target[2].x=-75;
		target[2].y=-25;
		target[2].visible=true;
	}
	if (menu.help>185 && menu.help<=186)
	{
		target[0].visible=false;
		target[1].visible=false;
		target[2].visible=false;
		asteroid[14].x=0;
		asteroid[14].y=0;
		asteroid[14].xspeed=0;
		asteroid[14].yspeed=5;
		asteroid[14].visible=true;
		asteroid[14].size=2;

	}
	if (menu.help>195 && menu.help<=196)
	{
		bt1[0].x=0;
		bt1[0].y=-50;
		bt1[0].yspeed=10;
		bt1[0].xspeed=0;
		bt1[0].visible=true;
		bt1[0].active=5;
	}
	if (menu.help>235 && menu.help<=236)
	{
		user.x=-225;
		user.y=-100;
		user.xspeed=0;
		user.yspeed=0;
		user.dir=85;
		user.health=100;
		user.visible=true;
	}
	if (menu.help>236 && menu.help<285)
	{
		user.xspeed+=cos(user.dir*(PI/180))*thrust;
		user.yspeed+=sin(user.dir*(PI/180))*thrust;      
		Thrust(user);
	}
	if (menu.help==286)
	{
		user.x=-100;
		user.y=-205;
		user.xspeed=0;
		user.yspeed=0;
		user.dir=80;
		user.health=50;
		user.visible=true;

		user2.x=200;
		user2.y=-205;
		user2.xspeed=0;
		user2.yspeed=0;
		user2.dir=100;
		user2.health=100;
		user2.visible=true;
		user2.range=240;
		user2.prime=30;

		int i;
		for (i=0; i<50; i++)
			asteroid[i].visible=false;
	}
	if (menu.help>310 && menu.help<335)
	{
		user2.xspeed+=cos(user2.dir*(PI/180))*thrust;
		user2.yspeed+=sin(user2.dir*(PI/180))*thrust;      
		Thrust(user2);

		if (menu.help==315)
			user2.dir=111;

		if (menu.help==320)
			Fire(user2);

		if (menu.help>=330)
			user2.dir+=-5;        
	}
	if (menu.help>405 && menu.help<435)
	{
		user2.xspeed+=cos(user2.dir*(PI/180))*thrust;
		user2.yspeed+=sin(user2.dir*(PI/180))*thrust;      
		Thrust(user2);
	}
	if (menu.help>470 && menu.help<475)
	{
		user2.dir=85;
		user2.xspeed+=cos(user2.dir*(PI/180))*thrust;
		user2.yspeed+=sin(user2.dir*(PI/180))*thrust;      
		Thrust(user2);

		if (menu.help==471)
			Fire(user);
	}
	if (menu.help>470 && menu.help<490)
	{
		user.xspeed+=cos(user.dir*(PI/180))*thrust;
		user.yspeed+=sin(user.dir*(PI/180))*thrust;      
		Thrust(user);
	}
	if (menu.help>515 && menu.help<535)
	{
		user.xspeed+=cos(user.dir*(PI/180))*thrust;
		user.yspeed+=sin(user.dir*(PI/180))*thrust;      
		Thrust(user);
	}
	if (menu.help==565)
		CurrentLoop=MenuInit;


	if (menu.help!=1
		&& menu.help!=3
		&& menu.help!=35
		&& menu.help!=45
		&& menu.help!=60
		&& menu.help!=70
		&& menu.help!=100
		&& menu.help!=125
		&& menu.help!=150
		&& menu.help!=170
		&& menu.help!=175
		&& menu.help!=185
		&& menu.help!=195
		&& menu.help!=235
		&& menu.help!=285
		&& menu.help!=300
		&& menu.help!=335
		&& menu.help!=455
		&& menu.help!=490
		&& menu.help!=535
		&& menu.help!=550
		&& menu.help!=560)
	{
		menu.help++;
		Move();
	}
	else
	{
		if (KeyState(VK_RETURN))
			menu.help++;
	}

	DrawAll();

	return;
}
