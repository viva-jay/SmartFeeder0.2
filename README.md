## 프로젝트 설명

기존의 ver 0.1의 문제점을 해결하고 MAX-4466 센서를 이용하여 반려견의 소리가 일정이상 지속되면 등록된 스마트폰으로 PUSH하여 급식과 음악 재생, 반려견의 모습 확인 가능하도록 하는 반려견 급식기 입니다.

### 기존 프로젝트의 문제점

* mobile과 급식기가 1:1로 통신가능하고, 같은 네트워크 안에서만 제어가 가능하다.



## 개발 환경

* RaspberryPi3, Naber Cloud Platform, MongoDB, node.js, Android ,ubuntu 12.04

* Java Script, C, Java

  

## 구현기능

* Server
  * 짧은 이벤트 요청을 빠르게 처리하기 위하여 Node.js 사용, 클라이언트의 쉬운 확장을 고려하여 MQTT 프로토콜을 사용하여 Feeder를 제어
  * 급식기와 Mobile사이의 영상 연결을 위해 WebRTC프로토콜을 사용
  * 로그의 빠른 저장을 위하여 MongoDB를 사용

* Client

  * 라즈베리파이와 연결된 마이크모듈(MAX4466)로 입력되는 사운드 아날로그 신호를 받기위ADC (MCP 3008)를 사용. SPI통신과 서보모터 제어를 위해 wiringPi라이브러리를 사용하여 구현.

  * 서버와 MQTT통신을 위해 클라이언트는 paho라이브러리, 브로커는 mosquitto를 이용하여 개발

  * 급식기와 영상 연결, 모터제어, 음악 재생을 가진 안드로이드 어플을 구현, HTTP통신을 통하여 서버와 연결됨, 스트리밍서버와 연결된 화면은 웹뷰를 이용.

    

## 시스템 구성도



## 동영상
[![feeder 동영상](http://img.youtube.com/vi/TG81ZKylOGE/0.jpg)](https://youtu.be/TG81ZKylOGE?t=0s) 




