
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


#define WINDOW_WIDTH (640)
#define WINDOW_HEIGHT (480)
#define SPEED (300)
#define h_addr h_addr_list[0]


void error(char *msg){

	perror(msg);
	exit(0);
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

	SDL_Rect dest,plank_dest,plank2_dest;
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


	int sock,length,n;
	//struct sockaddr_in server,from;
	struct sockaddr_in6 server,from;
	struct hostent *hp;
	char buffer[256];

	int p1buf[4];
	int p2buf[4];

	//sock=socket(AF_INET,SOCK_DGRAM,0);
	sock=socket(AF_INET6,SOCK_DGRAM,0);

	if(sock<0){
		error("error occured while creating socket.\n");
	}

	//server.sin_family=AF_INET;
	//hp=gethostbyname("localhost");
	
	//if(hp==0){
	//	error("Unknown host specified.\n");
	//}

	//bcopy((char *)hp->h_addr,(char *)&server.sin_addr,hp->h_length);
	//int p=atoi(argv[1]);
	//if(p==1)
	//server.sin_port=htons(atoi("1337"));
	//if(p==2)
	//server.sin_port=htons(atoi("1338"));
	//length=sizeof(struct sockaddr_in);

	memset(&server, 0, sizeof(server));
	server.sin6_family=AF_INET6;
	int p=atoi(argv[1]);
	if(p==1)
		server.sin6_port=htons(atoi("9001"));
	if(p==2)
		server.sin6_port=htons(atoi("9002"));
	char *host_ip=argv[2];
	inet_pton(AF_INET6, host_ip, &(server.sin6_addr));
	length=sizeof(struct sockaddr_in6);

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
			printf("%d\n",score);
			x_pos=WINDOW_WIDTH-dest.w-plank_dest.w;
			x_vel=-x_vel;
		}

	


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
			printf("player 2:%d\n",score);
			x_pos=plank2_dest.w;
			x_vel=-x_vel;
		}

	}if(p==2){	

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
		dest.y=(int)y_pos;
		dest.x=(int)x_pos;
		plank_y_pos+=plank_y_vel/50;
		plank2_y_pos+=plank2_y_vel/50;
		plank_dest.y=(int)plank_y_pos;
		plank_dest.x=(int)plank_x_pos;
		plank2_dest.y=(int)plank2_y_pos;
		plank2_dest.x=(int)plank2_x_pos;
		
		if(p==1){
			p1buf[0] = htonl(dest.x);
			p1buf[1] = htonl(dest.y);
			p1buf[2] = htonl(plank_dest.x);
			p1buf[3] = htonl(plank_dest.y);
			//printf("adsad\n",n);
			//bzero(buffer,256);
			//sprintf(buffer,"%d;%d;%d;%d",dest.x,dest.y,plank_dest.x,plank_dest.y);
			//n=sendto(sock,buffer,strlen(buffer),0,&server,length);
			printf("sending to server\n");
			n=sendto(sock, p1buf, sizeof(int)*4, 0, &server, length);
			//printf("%s\n",buffer);
			if(n<0){
				error("error occured while sending.\n");
			}

			//n=recvfrom(sock,buffer,256,0,&from,&length);
			printf("receiving from server\n");
			n=recvfrom(sock,p1buf,sizeof(int)*2,0,&from,&length);
			if(n<0){
				error("error occured while recieving in p2.\n");
			}
			plank2_dest.x = ntohl(p1buf[0]);
			plank2_dest.y = ntohl(p1buf[1]);
			printf("got %d %d\n", plank2_dest.x, plank2_dest.y);

			//write(1,"Got acknowledgement: ",20);
			//write(1,buffer,n);
			//sscanf(buffer,"%d;%d;",&plank2_dest.x,&plank2_dest.y);

			printf("\n");
		}

		if(p==2){
			p2buf[0] = htonl(plank2_dest.x);
			p2buf[1] = htonl(plank2_dest.y);

			//bzero(buffer,256);
			//sprintf(buffer,"%d;%d",plank2_dest.x,plank2_dest.y);
			//n=sendto(sock,buffer,strlen(buffer),0,&server,length);
			printf("sending to server\n");
			n=sendto(sock,p2buf,sizeof(int)*2,0,&server,length);
			if(n<0){
				error("error occured while sending.\n");
			}

			//n=recvfrom(sock,buffer,256,0,&from,&length);
			printf("receiving from server\n");
			n=recvfrom(sock,p2buf,sizeof(int)*4,0,&from,&length);
			if(n<0){
				error("error occured while recieving in p2.\n");
			}

			dest.x = ntohl(p2buf[0]);
			dest.y = ntohl(p2buf[1]);
			plank_dest.x = ntohl(p2buf[2]);
			plank_dest.y = ntohl(p2buf[3]);
			//write(1,"Got acknowledgement: ",20);
			//write(1,buffer,n);
			//sscanf(buffer,"%d;%d;%d;%d",&dest.x,&dest.y,&plank_dest.x,&plank_dest.y);
			
			printf("got %d %d %d %d\n", dest.x, dest.y, plank_dest.x, plank_dest.y);
			printf("\n");
		}
		SDL_RenderClear(rend);
		SDL_RenderCopy(rend,tex,NULL,&dest);
		SDL_RenderCopy(rend,pla,NULL,&plank_dest);
		SDL_RenderCopy(rend,pla2,NULL,&plank2_dest);
		SDL_RenderPresent(rend);
		SDL_Delay(1000/60);


	}

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(win);
	SDL_Quit();
	
}