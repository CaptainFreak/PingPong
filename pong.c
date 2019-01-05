
/*
		2D PingPong(Multiplayer)

--UDP based Server-Client model
Author:CaptainFreak

*/


#include <stdio.h>
#include <SDL2/SDL.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>

#define WINDOW_WIDTH (640)
#define WINDOW_HEIGHT (480)
#define SPEED (300)
#define h_addr h_addr_list[0]

struct player_thread_info {
	int player_number;
	int recv_socket;
	int *write_dest[4]; // array of pointers, each pointer points to an int
};

// physics globals
SDL_Rect dest,plank_dest,plank2_dest;

// "connected" will allow main thread to begin only when there is a connection
pthread_mutex_t connected_mutex = PTHREAD_MUTEX_INITIALIZER;
int connected = 0;
// "physics" will allow "updateOpponentPosition" thread to update physics
// "physics" will also allow main thread to read physics
pthread_mutex_t physics_mutex = PTHREAD_MUTEX_INITIALIZER;

void error(char *msg){

	perror(msg);
	exit(0);
}

// thread that gets opponent position from the host
void *updateOpponentPosition(void *ptr) {
	int buf[4];
	int n, looplen;
	struct sockaddr_in6 server;
	int servlen = sizeof(struct sockaddr_in6);
	struct player_thread_info *pti = (struct player_thread_info *) ptr;
	
	// get "connection granted" = [-1, -1, -1, -1], and begin main game loop
	n = recvfrom(pti->recv_socket, buf, sizeof(int) * 4, 0,
		&server, &servlen);
	pthread_mutex_lock(&connected_mutex);
	connected = 1;
	pthread_mutex_unlock(&connected_mutex);

	// player 1 only needs to write to two locations; p2 writes to four
	looplen = pti->player_number * 2;
	// side game loop, listens for recv
	while(1) {
		n = recvfrom(pti->recv_socket, buf, sizeof(int) * 4, 0,
			&server, &servlen);
		pthread_mutex_lock(&physics_mutex);
		for(int i = 0; i < looplen; i++) {
			*(pti->write_dest[i]) = ntohl(buf[i]);
		}
		pthread_mutex_unlock(&physics_mutex);
	}
}

int main(int argc, char *argv[]){

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0){
		printf("Error initializing SDL:%s\n",SDL_GetError());
		return 1;
	}

	SDL_Window* win=SDL_CreateWindow("PingPong Multiplayer By CaptainFreak",
									SDL_WINDOWPOS_CENTERED,
									SDL_WINDOWPOS_CENTERED,
									WINDOW_WIDTH,WINDOW_HEIGHT,0);

	if(!win){
		printf("Error creating window:%s\n",SDL_GetError());
		SDL_Quit();
		return 1;
	}

	Uint32 render_flags=SDL_RENDERER_ACCELERATED;
	SDL_Renderer* rend=SDL_CreateRenderer(win,-1,render_flags);
	if(!rend){
		printf("Error creating renderer:%s\n",SDL_GetError());
		SDL_DestroyWindow(win);
		SDL_Quit();
		return 1;
	}

	SDL_Surface* surface=IMG_Load("./src/ball.png");
	SDL_Surface* plank=IMG_Load("./src/plank.png");
	SDL_Surface* plank2=IMG_Load("./src/plank.png");
	if(!surface || !plank || !plank2){
		printf("Error creating surface:%s\n",SDL_GetError());
		SDL_DestroyRenderer(rend);
		SDL_DestroyWindow(win);
		SDL_Quit();
		return 1;
	}

	SDL_Texture* tex=SDL_CreateTextureFromSurface(rend,surface);
	SDL_FreeSurface(surface);
	SDL_Texture* pla=SDL_CreateTextureFromSurface(rend,plank);
	SDL_FreeSurface(plank);
	SDL_Texture* pla2=SDL_CreateTextureFromSurface(rend,plank2);
	SDL_FreeSurface(plank2);
	if(!tex || !pla || !pla2){
		printf("Error while creating texture:%s\n",SDL_GetError());
		SDL_DestroyRenderer(rend);
		SDL_DestroyWindow(win);
		SDL_Quit();
		return 1;
	}

	//SDL_Rect dest,plank_dest,plank2_dest;
	SDL_QueryTexture(tex,NULL,NULL,&dest.w,&dest.h);
	dest.w/=30;
	dest.h/=30;
	SDL_QueryTexture(pla,NULL,NULL,&plank_dest.w,&plank_dest.h);
	plank_dest.w/=16;
	plank_dest.h/=16;
	SDL_QueryTexture(pla2,NULL,NULL,&plank2_dest.w,&plank2_dest.h);
	plank2_dest.w/=16;
	plank2_dest.h/=16;

	float x_pos=(WINDOW_WIDTH-dest.w)/2;
	float y_pos=(WINDOW_HEIGHT-dest.y)/2;
	
	float plank_x_pos=(WINDOW_WIDTH-plank_dest.w);
	float plank_y_pos=(WINDOW_HEIGHT-plank_dest.y)/2;

	float plank2_x_pos=0;
	float plank2_y_pos=(WINDOW_HEIGHT-plank2_dest.y)/2;
	
	float x_vel=SPEED;
	float y_vel=SPEED;

	float plank_y_vel=0;
	float plank2_y_vel=0;

	int up=0;
	int down=0;
	int up2=0;
	int down2=0;

	int close_req=0;
	int score=0;
	int score2=0;

	int p1buf[4], p2buf[4];

	// network part
	int sock, length, n;
	struct sockaddr_in6 server,from;
	char buffer[256];

	sock = socket(AF_INET6, SOCK_DGRAM, 0);
	if(sock < 0){
		error("error occured while creating socket.\n");
	}

	// fill in server information, so we can send to server
	memset(&server, 0, sizeof(server));
	server.sin6_family = AF_INET6;
	int p = atoi(argv[1]);
	if(p==1)
		server.sin6_port = htons(atoi("9001"));
	if(p==2)
		server.sin6_port = htons(atoi("9002"));
	char *host_ip = argv[2];
	inet_pton(AF_INET6, host_ip, &(server.sin6_addr));
	length = sizeof(struct sockaddr_in6);

	// threading part
	pthread_t recv_thread;
	int ret;
	struct player_thread_info pti;
	
	pti.player_number = p;
	pti.recv_socket = sock;
	if(p == 1) {
		pti.write_dest[0] = &(plank2_dest.x);
		pti.write_dest[1] = &(plank2_dest.y);
		pti.write_dest[2] = NULL;
		pti.write_dest[3] = NULL;
	} else if(p == 2) {
		pti.write_dest[0] = &(dest.x);
		pti.write_dest[1] = &(dest.y);
		pti.write_dest[2] = &(plank_dest.x);
		pti.write_dest[3] = &(plank_dest.y);
	}
	
	ret = pthread_create(&recv_thread, NULL, updateOpponentPosition,
		(void *) &pti);

	// repeatedly tell server you are willing to connect
	int ready_message[4] = { htonl(-1), htonl(-1), htonl(-1), htonl(-1) };
	pthread_mutex_lock(&connected_mutex);
	while(connected == 0) {
		pthread_mutex_unlock(&connected_mutex);
		n = sendto(sock, ready_message, sizeof(int) * 4, 0,
			&server, length);
		pthread_mutex_lock(&connected_mutex);
	}
	pthread_mutex_unlock(&connected_mutex);

	while(!close_req){
		SDL_Event event;
		while(SDL_PollEvent(&event)){
		if(p==1){
			switch (event.type){
				case SDL_QUIT:
					close_req=1;
					break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.scancode){
						case SDL_SCANCODE_UP:
							up=1;
							break;
						case SDL_SCANCODE_DOWN:
							down=1;
							break;
					}
					break;
				case SDL_KEYUP:
					switch(event.key.keysym.scancode){
						case SDL_SCANCODE_UP:
							up=0;
							break;
						case SDL_SCANCODE_DOWN:
							down=0;
							break;
		
					}	
					break;
						
			}
		}
		if(p==2){
			switch (event.type){
				case SDL_QUIT:
					close_req=1;
					break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.scancode){

						case SDL_SCANCODE_W:
							up2=1;
							break;
						case SDL_SCANCODE_S:
							down2=1;
							break;		
					}
					break;
				case SDL_KEYUP:
					switch(event.key.keysym.scancode){

						case SDL_SCANCODE_W:
							up2=0;
							break;
						case SDL_SCANCODE_S:
							down2=0;
							break;			
					}	
					break;
						
			}
		}

	}


	if(p==1){
		// update the ball
		if(x_pos<=0){
			x_pos=0;
			x_vel=-x_vel;
		}
		if(y_pos<=0){
			y_pos=0;
			y_vel=-y_vel;
		}
		if(x_pos>=WINDOW_WIDTH-dest.w){
			x_pos=WINDOW_WIDTH-dest.w;
			x_vel=-x_vel;
		}
		if(y_pos>=WINDOW_HEIGHT-dest.h){
			y_pos=WINDOW_HEIGHT-dest.h;
			y_vel=-y_vel;
		}
		if(x_pos>=WINDOW_WIDTH-dest.w-plank_dest.w && (y_pos>=plank_y_pos-dest.h && y_pos<=plank_y_pos+plank_dest.h)){
			score++;
			//printf("%d\n",score);
			x_pos=WINDOW_WIDTH-dest.w-plank_dest.w;
			x_vel=-x_vel;
		}

	

		// update player 1 plank
		if(plank_y_pos<=0){
			plank_y_pos=0;
		}
		if(plank_y_pos+plank_dest.h>=WINDOW_HEIGHT){
			plank_y_pos=WINDOW_HEIGHT-plank_dest.h;
		}

		if(up && !down){
			plank_y_vel=-SPEED;
		}
		else if(!up && down){
			plank_y_vel=SPEED;
		}else{
			plank_y_vel=0;
		}

		if(x_pos<=plank2_dest.w && (y_pos>=plank2_y_pos-dest.h && y_pos<=plank2_y_pos+plank2_dest.h)){
			score2++;
			//printf("player 2:%d\n",score);
			x_pos=plank2_dest.w;
			x_vel=-x_vel;
		}

	}if(p==2){	
		// update player 2 plank
		if(plank2_y_pos<=0){
			plank2_y_pos=0;
		}
		if(plank2_y_pos+plank2_dest.h>=WINDOW_HEIGHT){
			plank2_y_pos=WINDOW_HEIGHT-plank2_dest.h;
		}

		if(up2 && !down2){
			plank2_y_vel=-SPEED;
		}
		else if(!up2 && down2){
			plank2_y_vel=SPEED;
		}else{
			plank2_y_vel=0;
		}

	}	

		x_pos+=x_vel/60;
		y_pos+=y_vel/60;
		plank_y_pos+=plank_y_vel/50;
		plank2_y_pos+=plank2_y_vel/50;
		
		// critical region start
		pthread_mutex_lock(&physics_mutex);

		// this thread updates one part of physics data
		// the other thread (the listener) updates the second part
		if(p == 1) {
			dest.y=(int)y_pos;
			dest.x=(int)x_pos;
			plank_dest.y=(int)plank_y_pos;
			plank_dest.x=(int)plank_x_pos;
		} else if(p == 2) {
			plank2_dest.y=(int)plank2_y_pos;
			plank2_dest.x=(int)plank2_x_pos;
		}

		if(p==1){
			// send position data of ball and player 1 to server
			
			// TODO: locks
			// prepare data
			p1buf[0] = htonl(dest.x);
			p1buf[1] = htonl(dest.y);
			p1buf[2] = htonl(plank_dest.x);
			p1buf[3] = htonl(plank_dest.y);
			// send data
			n = sendto(sock, p1buf, sizeof(int) * 4, 0, &server, length);
			if(n < 0){
				error("error occured while sending.\n");
			}
		}

		if(p==2){
			// send position data of player 2 to server
			
			// prepare data
			p2buf[0] = htonl(plank2_dest.x);
			p2buf[1] = htonl(plank2_dest.y);
			p2buf[2] = 0; // nothing
			p2buf[3] = 0; // nothing
			// send data
			n=sendto(sock, p2buf, sizeof(int) * 4, 0, &server, length);
			if(n < 0){
				error("error occured while sending.\n");
			}
		}

		SDL_RenderClear(rend);
		SDL_RenderCopy(rend,tex,NULL,&dest);
		SDL_RenderCopy(rend,pla,NULL,&plank_dest);
		SDL_RenderCopy(rend,pla2,NULL,&plank2_dest);
		SDL_RenderPresent(rend);
		SDL_Delay(1000/60);

		// critical region end
		pthread_mutex_unlock(&physics_mutex);
	}

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(win);
	SDL_Quit();

	// threading end
	pthread_join(recv_thread, NULL);	
}