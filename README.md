### \[개요\]
select 모델을 적용한 소켓 프로그래밍을 통해 싱글 스레드 기반의 네트워크 라이브러리이다.
이벤트 기반의 콘텐츠 코드 호출(OnAccept, OnRecv)에서 나아가 RPC 개념을 적용하여 컨텐츠 코드에서는 패킷에 대한 직렬화/역직렬화, 파싱까지 추상화된 RPC 함수를 통해 컨텐츠 코드를 수행 및 패킷 송신을 할 수 있도록 하였다.

<p align="center">
  <img src="https://velog.velcdn.com/images/jinh2352/post/2b588124-b3f6-4bde-9837-83ac4a7d9134/image.png" width="800">
</p>

싱글 스레드 기반이기에 세션에 대한 관리는 JNetLibrary에 비해 간단하다. IOCP 모델 적용 이전에 구현한 라이브러리로 주된 목적은 네트워크 세션 연결 및 수명 관리, 송수신 그리고 RPC 로직에 대한 이해이다. 

#### RPC 자동화 툴
서버-클라이언트 간 주고받을 메시지에 대한 명세는 json 파일에 기입하며, 자체적인 'JPD(JiniNet Protocol Definition)'을 따른다. json 파일이 아닌 유니티 UI 시스템을 통한 명세 툴을 구현하였다. 

<p align="center">
  <img src="https://github.com/user-attachments/assets/79175705-8f2f-43bd-89ff-585d2a9c1a2e" width="800">
</p>

https://github.com/JINs-software/JPD_Compiler_IDE

위 링크를 통해 JiniNe 라이브러리의 JPD 컴파일 및 명세 자동화 툴 그리고 JiniNet의 RPC에 대한 요약된 설명을 확인할 수 있다. 

#### JiniNet's RPC 적용 프로젝트 및 RPC 설명
* JiniNet's RPC server(c++) 설명: https://github.com/JINs-software/MMO_Fighter
* JiniNet's RPC client(c#) 설명: https://github.com/JINs-software/MMO_Fighter_Client

---

### \[코어 라이브러리와 주요 멤버 함수\]
#### JNetworkCore
JNerwork는 하위의 클래스의 네트워크 송수신 로직과 컨텐츠 코드의 배치 수행을 'FrameMove'라는 함수로 호출 및 관리하는 템플릿과 같은 클래스이다.

<img src="https://velog.velcdn.com/images/jinh2352/post/ac0780db-45c2-4a3c-9396-dd3e5188e876/image.png" width="500">

* JNetworkCore::FrameMove(uint16 frameDelta) : void
  
  <img src="https://velog.velcdn.com/images/jinh2352/post/6954052a-c62a-44c1-b0c8-045b1d4c7b8e/image.png" width="300">

#### JNetCoreServer

<img src="https://velog.velcdn.com/images/jinh2352/post/c659124a-ea9b-4931-9039-cdc0680608b3/image.png" width="400">
...
<img src="https://velog.velcdn.com/images/jinh2352/post/145b23b5-6025-4467-ae34-29f351f8f492/image.png" width="400">

* Attach 계열 함수
라이브러리 단에서 호출되는, 즉 라이브리러에 의존하는 코드들이 있다. 라이브러리가 제공하는 클래스를 상속한 후 가상 함수를 재정의하여 오버라이딩되도록 하는 방식이다. 연결 수립 및 세션 생성과 연결 종료와 같은 이벤트를 다루기 위해 이벤트 핸들러 객체를 라이브러리 단에 부착한다. RPC 자동화 툴을 통해 생성된 RPC 코드에서 Stub(called by library) 함수를 재정의 한 객체를 등록할 수 있으며, proxy 함수를 호출하기 위해 proxy 객체 또한 부착할 수 있다.
마지막으로 이동과 같은 일괄적으로 배치(batch)로 수행되어야 할 작업을 정의한 상속 클래스의 객체를 부착할 수 있다.
	
* Receive/Send
상위 단에서 호출될 Receive/Send는 select 모델을 통한 네트워크 송수신을 수행한다.

  <img src="https://velog.velcdn.com/images/jinh2352/post/aa2b9a1c-6c91-41fe-91e9-49f0b7205d29/image.png" width="250">

	* receiveSet: 리슨 소켓과 클라이언트 소켓, fd_set 구조체를 FD_SET 함수를 호출함으로써 수신을 대기한다.
	* receive: FD_ISSET을 통해 수신 버퍼를 확인하며 수신 가능 시 이를 수신하여 OnClientJoin(리슨 소켓), 또는 OnRecv 이벤트 함수를 호출되거나 RPC 모드인 경우 라이브러리의 스텁 조상 클래스인 JNetStub의 ProcessReceivedMessage 함수가 호출되어 자식 스텁 클래스에 정의된 함수들로 파생된다. 
	* sendSet: 송신이 가능한 상황인지 개별 클라이언트 소켓들을 대상으로 FD_SET을 호출하여 송신 버퍼를 비동기적으로 확인
	* send: 송신 가능 시 세션 별 송신 버퍼에 적제된 송신 데이터를 송신

#### JNetProxy
<img src="https://velog.velcdn.com/images/jinh2352/post/780f7efa-6db7-4cb6-85e5-5ea589661a3c/image.png" width="400">
과거의 버전에는 라이브러리 단의 Send 함수를 호출하는 랩퍼 함수인 Send를 활용하였다. 그러나 추가적인 복사로 인한 성능 저하가 발견되어 라이브러리 클래스에서 JNetProxy를 프랜드 선언하고, 프록시는 라이브러리의 세션 객체에 직접 접근하여 송신 버퍼를 참조할 수 있다. 
싱글 스레드 구조인 라이브러리 코어이기에 세션 생성 및 삭제에 있어서의 동기화 문제가 없어 적용하였다. 
	
#### JNetStub
RPC 자동화 툴로 작성될 코드 중 스텁 클래스들의 부모 클래스가 된다. 
<img src="https://velog.velcdn.com/images/jinh2352/post/fb4d3f60-4ae8-47e3-a08f-0791f9c963ca/image.png" width="500">

ProcessReceivedMessage 함수는 라이브러리 단에서 호출된다. 이 함수를 통해 컨텐츠 코드가 삽입된 Stub 자식 클래스들의 함수가 오버라이딩으로 호출된다. Get 계열의 함수는 메시지 네임 스페이스 및 메시지 관리를 위해 사용된다. 

#### 기타
* 세션 관련 이벤트를 처리할 수 있는 가상 함수가 선언된 JNetEventHandler 클래스
  
  <img src="https://velog.velcdn.com/images/jinh2352/post/73f5ca85-d83b-4d46-9638-0aee894e9d93/image.png" width="400">

* 라이브러리에 의존하여 배치(batch)로 호출될 가상 함수들이 선언된 JNetBatchProcess
  
  <img src="https://velog.velcdn.com/images/jinh2352/post/a883ed01-679a-460c-950e-476ec5d47923/image.png" width="400">
