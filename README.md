# micro-ros
- Reference [micro-ros](https://github.com/micro-ROS/micro_ros_espidf_component/tree/jazzy)

# development environment
## make docker file
Create Dockerfile
```dockerfile
FROM espressif/idf:release-v5.5

# 1. ESP-IDF가 제공하는 파이썬 가상환경 내부의 pip를 정확히 지칭하여 설치
RUN /opt/esp/python_env/idf5.5_py3.12_env/bin/pip install \
    catkin_pkg \
    colcon-common-extensions \
    lark

# 2. 이후 idf.py 명령이나 micro-ROS 빌드 툴이 컨테이너 실행 시 자동 로드되도록 설정
SHELL ["/bin/bash", "-il", "-c"]

ARG USERNAME=ubuntu
ARG USER_UID=1000
ARG USER_GID=1000
# Sudoer permissions for the user
RUN echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Set the user and working directory
USER ${USERNAME}
WORKDIR /home/${USERNAME}

```
## docker build
```bash
docker build -t microros/esp-idf:v5.5 .
```

## micro-ros-espidf-component
```bash
mkdir uros_ws
cd uros_ws
git clone -b jazzy https://github.com/micro-ROS/micro_ros_espidf_component.git
```

## docker run
```bash
cd uros_ws
docker run -it --rm -v $(pwd)/micro_ros_espidf_component:/home/ubuntu/uros_ws -v /dev:/dev --privileged --net=host microros/esp-idf:v5.5
```

# micro-ros agent
Open a new terminal

```bash
# UDPv4 micro-ROS Agent
docker run -it --rm --net=host microros/micro-ros-agent:jazzy udp4 --port 8888 -v6
```



