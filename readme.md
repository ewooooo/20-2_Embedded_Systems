# 문자 디바이스 드라이버 개발
-	드라이버 이름: BufferedMem
-	개발환경: Ubuntu 16.04.7
-	BufferedMem 장치 Major Number: 275 
-	사용자 write한 정보 내부 버퍼에 유지
-	내부 버퍼 크기(N) 사용자가 변경 가능
-	사용자 입력 데이터가 최대 내부 버퍼 크기를 초과한 경우 최초 입력된 데이터부터 삭제
-	사용자가 Read할 경우 M 바이트만 전달
-	사용자 Read Buffer Size (M) 크기 변경가능
-	Read 된 데이터는 내부 버퍼에서 제거
-	N, M 변경은 ioctl 이용
-	Device Driver를 등록할 때 N, M 사용자 지정 가능
-	내부 버퍼는 kfifo로 구현
-	디바이스 등록 및 제거 시 로그 메시지 출력
- 등록 시: Device driver registered 출력 
- 제거 시: Device driver unregistered 출력
 

## 작성 프로그램: 
-   BufferedMem Device Driver – BufferedMem 장치의 device driver 
-	ch_read_buffer_size – 장치로부터 read할 때 읽어 들이는 데이터 크기 조절 프로그램 
-	ch_wirte_buffer_size – write된 데이터를 저장하는 내부 버퍼 크기 조절 프로그램 
-	Makefile 


## 상세 기능
-	내부 버퍼 크기 가변형 구현 
-	Read 기능
-	Write 기능
-	N/M 크기 초기화
-	N 변경 기능
-	M 변경 기능 
- 로그 출력 기능 
-	디바이스 파일 생성 
 
