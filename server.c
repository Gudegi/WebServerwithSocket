#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h> //strstr ,strlen
//각 확장자에 대응하는 Content-Type 값
char* AUDIO = "audio/mpeg";   
char* HTML = "text/html;charset=UTF-8";
char* PDF = "application/pdf";
char* PNG = "image/png";
char* GIF = "image/gif";

void error(char *message){ //에러처리용
    perror(message);
    exit(1);
}
void res_header(int socket, const char* contenttype, const char* filename ){             //http 응답 헤더(소켓, 컨텐츠타입, 파일이름)
    size_t filesize ;    //파일사이즈
    char buf[1024];  //buf
    char * headers[6] = {   //응답 헤더 
        "HTTP/1.1 200 OK\r\n",      
        "Host: /index.html\r\n",
        "Connection: keep-alive\r\n",
        "Date:2020-06-07\r\n"
    };
    FILE* fp = fopen(filename,"rb"); //파일 사이즈 값 넣기위하여
    fseek(fp,0,SEEK_END); // SEEK_END로 파일 끝 가리키게
    filesize = ftell(fp); // 파일 크기
    fclose(fp); //파일 스트림 닫기
    headers[4] = "Content-Length: %d\r\n"; //컨텐츠 크기 = 파일 사이즈 정수형이니까 %d
    headers[5] = "Content-Type: %s\r\n\r\n"; // 컨텐츠 타입 > 11 ~15줄, 문자열 %s
    sprintf(buf,headers[0]);   // sprintf > 서식맞춰서 buf에 입력
    send(socket, buf, strlen(buf), 0); //한줄씩 보내기
    sprintf(buf,headers[1]);
    send(socket, buf, strlen(buf), 0);
    sprintf(buf,headers[2]);
    send(socket, buf, strlen(buf), 0); 
    sprintf(buf,headers[3]);
    send(socket, buf, strlen(buf), 0); 
    sprintf(buf,headers[4],filesize);
    send(socket, buf, strlen(buf), 0);
    sprintf(buf,headers[5], contenttype);
    send(socket, buf, strlen(buf), 0);
}

void res_body(int socket, const char *path) {
    size_t buf ;  
    FILE* fp = fopen(path,"rb"); //link (파일경로)를 binary, 읽기전용으로 파일스트림 열기
    char* buffer = malloc(buf);         //크기가 얼마까지 갈지 몰라서 동적 메모리 할당 
    
    while(!feof(fp)) {  //파일 끝까지 읽기   feof는 끝에가면 TRUE반환,  !이므로 끝에서 FALSE> 탈출
    int len =fread(buffer, 1  ,buf,  fp);     //buffer에 읽은거 저장, 읽은 수 len에 저장
    send(socket, buffer, len, 0  ); //소켓에 읽은거 send buffer 에 있는거 len만큼 
    }
    free(buffer);  //할당 해제
    fclose(fp);  //파일스트림 닫기

}
int main(int argc, char **argv){ //메인
    size_t len;
    int sockfd, newsockfd, portno ;
    socklen_t clilen ;
    char buffer[1024];
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2){      //인자가 2개보다 작으면 오류
        fprintf(stderr, "ERROR, no port provided\n");
    }

    sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //소켓 생성 / ipv4,  ,tcp방식
    if (sockfd <0) error("socket open"); //리턴 -1 이면 소켓 오픈 오류

    bzero( (char *)&serv_addr, sizeof(serv_addr) ); //서버 주소 0으로 초기화
    portno = atoi(argv[1]);  // 서버 실행 시 인자로 받은 문자열 포트번호를 정수형으로 변환하여 portno에 저장
    serv_addr.sin_family = AF_INET;     //ipv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //ip주소
    serv_addr.sin_port = htons(portno); //포트번호
    
    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) <0) error("bind error"); //ip와 포트값 바인딩
    
    if(listen(sockfd, 5) <0 ) error("listen error");     //연결대기열 5개 생성
    

    clilen = sizeof(cli_addr);
    while(1){   //ctrl c 로만 탈출가능 연결 끝나도 accept로 돌아옮
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); //클라이언트와 연결된 소켓
    if (newsockfd < 0) error("accept error");    
    

    if( recv(newsockfd,buffer,len, 0) == -1) error("recv"); // 클라이언트가  accept 후 request를 보냄 > recv로 받아서 buffer에 저장 ,길이(크기)를 몰라서 size_t로 했는데 make시 오류가 나옵니다..
    printf("%s",buffer); // request가 담긴 buffer 출력
    

    if( strstr(buffer, ".html") != 0) {    // buffer 안에서 .html확장자 검색 > 알맞는 헤더 정보 주기 위해서 (strstr은 위치 반환이므로 >0)
    res_header(newsockfd,HTML,"index.html"); // 응답 헤더 보냄
    res_body(newsockfd, "index.html"); //헤더에 따른 바디 보냄
    }

    else if( strstr(buffer,".mp3") != 0){ //.mp3요청하면
    res_header(newsockfd,AUDIO,"Closer.mp3"); //AUDIO에 해당하는 Content-Type값 헤더로 보내고 Closer.mp3의 파일 크기 = Contene-Length
    res_body(newsockfd,"Closer.mp3");    //응답 바디에 Closer.mp3파일을 보냄
    }

    else if( strstr(buffer,".png") != 0){
    res_header(newsockfd,PNG,"birds.png");
    res_body(newsockfd,"birds.png");    
    }

    else if( strstr(buffer,".pdf") != 0){
    res_header(newsockfd,PDF,"cardnews.pdf");
    res_body(newsockfd,"cardnews.pdf");    
    }

    else if( strstr(buffer,".gif") != 0){
    res_header(newsockfd,GIF,"3333.gif");
    res_body(newsockfd,"3333.gif");    
    }
    
    
    close(newsockfd); 
    }
    
    close(sockfd); //while 빠져나오면 소켓 닫기
    
    return 0;

}